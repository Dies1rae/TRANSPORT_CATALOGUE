#include "serialization.h"


proto::Color color_proto(const svg::Color& color) {
    proto::Color result;
    if (std::holds_alternative<std::monostate>(color)) {
        result.set_isnone(true);
    } else if (std::holds_alternative<std::string>(color)) {
        result.set_name(std::get<std::string>(color));
    } else if (std::holds_alternative<svg::Rgb>(color)) {
        svg::Rgb rgb (std::get<svg::Rgb>(color));
        proto::Rgba protoRgb;
        protoRgb.set_hasopacity(false);
        protoRgb.set_r(rgb.red);
        protoRgb.set_g(rgb.green);
        protoRgb.set_b(rgb.blue);
        *result.mutable_rgba() = protoRgb;
    } else if (std::holds_alternative<svg::Rgba>(color)) {
        svg::Rgba rgba (std::get<svg::Rgba>(color));
        proto::Rgba protoRgba;
        protoRgba.set_hasopacity(true);
        protoRgba.set_r(rgba.red);
        protoRgba.set_g(rgba.green);
        protoRgba.set_b(rgba.blue);
        protoRgba.set_opacity(rgba.opacity);
        *result.mutable_rgba() = protoRgba;
    }
    return result;
}

void serialize(std::istream& in) {
    const json::Document rawRequests = json::Load(in);

    //DB file
    const json::Node& serializationSettings = rawRequests.GetRoot().AsMap().at("serialization_settings");
    std::string fileName = serializationSettings.AsMap().at("file"s).AsString();

    //Base requests
    const json::Node& baseRequests = rawRequests.GetRoot().AsMap().at("base_requests");
    std::vector<const json::Node*> stop_nodes;
    std::vector<const json::Node*> busroutes_nides;
    for (const json::Node& n: baseRequests.AsArray()) {
        if (n.AsMap().at("type"s).AsString() == "Stop"s) {
            stop_nodes.push_back(&n);
            continue;
        }
        if (n.AsMap().at("type"s).AsString() == "Bus"s) {
            busroutes_nides.push_back(&n);
            continue;
        }
        throw json::ParsingError("Base request parsing error"s);
    }
    database::TransportCatalogue catalogue;
    json_reader::fillDataBase(catalogue, stop_nodes, busroutes_nides);

    proto::TransportCatalogue db;

    struct Distance {
        std::string from;
        std::string to;
        int meters;
    };

    std::vector<Distance> distances;
    std::map<std::string, int> stopDict;

    //Stops
    int id = 0;
    for (const json::Node* node: stop_nodes) {
        auto* protoStop = db.add_stops();

        std::string name = node->AsMap().at("name"s).AsString();
        double latitude  = node->AsMap().at("latitude"s).AsDouble();
        double longitude  = node->AsMap().at("longitude"s).AsDouble();
        db.add_stop_names(name);
        stopDict.insert({name, id++});
        protoStop->set_id(id);
        proto::Coordinates proto_place;
        proto_place.set_lat(latitude);
        proto_place.set_lon(longitude);
        *protoStop->mutable_place() = proto_place;
        for(const auto& d: node->AsMap().at("road_distances"s).AsMap()) {
            distances.push_back({name,d.first,d.second.AsInt()});
        }
    }

    //Dist
    for (const Distance& distance: distances) {
        auto* protoDistance = db.add_distances();
        protoDistance->set_from(stopDict[distance.from]);
        protoDistance->set_to(stopDict[distance.to]);
        protoDistance->set_meters(distance.meters);
    }

    //BusRouts
    for (const json::Node* node: busroutes_nides) {
        auto* protoRoute = db.add_routes();

        std::string name = node->AsMap().at("name"s).AsString();
        bool isCycled = node->AsMap().at("is_roundtrip"s).AsBool();
        protoRoute->set_name(name);
        protoRoute->set_cycled(isCycled);

        for (const json::Node& n: node->AsMap().at("stops"s).AsArray()) {
            if (stopDict.find(n.AsString()) != stopDict.end()) {
                protoRoute->add_stop_id(stopDict[n.AsString()]);
            } else {
                throw std::invalid_argument("Invalid stop: "s + n.AsString());
            }
        }
    }

    //Render settings
    rndr::renderSettings renderSettings;
    if (rawRequests.GetRoot().AsMap().find("render_settings") != rawRequests.GetRoot().AsMap().end()) {
        renderSettings = json_reader::parseRenderSettings(rawRequests.GetRoot().AsMap().at("render_settings"));
    }
    proto::RenderSettings proto_render_settings;
    proto_render_settings.set_width(renderSettings.width_);
    proto_render_settings.set_height(renderSettings.height_);
    proto_render_settings.set_padding(renderSettings.padding_);
    proto_render_settings.set_line_width(renderSettings.line_width_);
    proto_render_settings.set_stop_radius(renderSettings.stop_radius_);
    proto_render_settings.set_bus_label_font_size(renderSettings.bus_label_font_size_);
    proto_render_settings.mutable_bus_label_offset()->set_x(renderSettings.bus_label_offset_.x);
    proto_render_settings.mutable_bus_label_offset()->set_y(renderSettings.bus_label_offset_.y);
    proto_render_settings.set_stop_label_font_size(renderSettings.stop_label_font_size_);
    proto_render_settings.mutable_stop_label_offset()->set_x(renderSettings.stop_label_offset_.x);
    proto_render_settings.mutable_stop_label_offset()->set_y(renderSettings.stop_label_offset_.y);
    *proto_render_settings.mutable_underlayer_color() = color_proto(renderSettings.underlayer_color_);
    proto_render_settings.set_underlayer_width(renderSettings.underlayer_width_);
    for (const auto& color: renderSettings.color_palette_) {
        auto* proto_color = proto_render_settings.add_color_palette();
        *proto_color = color_proto(color);
    }
    *db.mutable_res() = proto_render_settings;

    // Routing settings
    RoutingSettings routingSettings;
    if (rawRequests.GetRoot().AsMap().find("routing_settings") != rawRequests.GetRoot().AsMap().end()) {
        routingSettings = json_reader::parseRoutingSettings(rawRequests.GetRoot().AsMap().at("routing_settings"));
    }
    proto::RoutingSettings protoRoutingSettings;
    protoRoutingSettings.set_bus_wait_time(routingSettings.bus_wait_time_);
    protoRoutingSettings.set_bus_velocity(routingSettings.bus_velocity_);
    *db.mutable_rus() = protoRoutingSettings;

    database::transport_router finder(catalogue, routingSettings.bus_wait_time_, routingSettings.bus_velocity_);

    std::ofstream outFile(fileName, std::ios::binary);
    db.SerializeToOstream(&outFile);
}

void database_restore(const proto::TransportCatalogue& database, database::TransportCatalogue& catalogue) {
    //Stops
    for (int i = 0; i < database.stops_size(); ++i) {
        proto::Coordinates place = database.stops(i).place();
        elements::Stop* stop = new elements::Stop(database.stop_names(i),{place.lat(), place.lon()});
        catalogue.stop_add(*stop);
    }

    //Dist
    for (int i = 0; i < database.distances_size(); ++i) {
        proto::Distance distance = database.distances(i);
        catalogue.setDistance(database.stop_names(distance.from()), database.stop_names(distance.to()), distance.meters());
    }

    //BusRoutes
    for (int i = 0; i < database.routes_size(); ++i) {
        std::vector<elements::Stop*> stops;
        for (int j = 0; j < database.routes(i).stop_id_size(); ++j) {
            stops.push_back(catalogue.stop_byname(database.stop_names(database.routes(i).stop_id(j))));
        }
        catalogue.rout_add(database.routes(i).name(), stops, database.routes(i).cycled());
    }
}

inline svg::Color protoToColor(const proto::Color& color) {
    auto dataType = color.data_case();

   if (dataType == proto::Color::kIsNone) {
       return {};
   } else if (dataType == proto::Color::kName) {
       return svg::Color(color.name());
   } else if (dataType == proto::Color::kRgba) {
       auto components = color.rgba();
       if (components.hasopacity()) {
           return svg::Color(svg::Rgba(components.r(), components.g(), components.b(),
                                       components.opacity()));
       } else {
           return svg::Color(svg::Rgb(components.r(), components.g(), components.b()));
       }
   }
   return {};
}

void settings_restore(const proto::TransportCatalogue& database, rndr::renderSettings& renderSettings) {
    renderSettings.width_ = database.res().width();
    renderSettings.height_ = database.res().height();
    renderSettings.padding_ = database.res().padding();
    renderSettings.line_width_ = database.res().line_width();
    renderSettings.stop_radius_ = database.res().stop_radius();
    renderSettings.bus_label_font_size_ = database.res().bus_label_font_size();
    renderSettings.bus_label_offset_ = {database.res().bus_label_offset().x(), database.res().bus_label_offset().y()};
    renderSettings.stop_label_font_size_ = database.res().stop_label_font_size();
    renderSettings.stop_label_offset_ = {database.res().stop_label_offset().x(), database.res().stop_label_offset().y()};
    renderSettings.underlayer_color_ = protoToColor(database.res().underlayer_color());
    renderSettings.underlayer_width_ = database.res().underlayer_width();
    for (int i = 0; i < database.res().color_palette_size(); ++i) {
        renderSettings.color_palette_.push_back(protoToColor(database.res().color_palette(i)));
    }
}

void settings_restore(const proto::TransportCatalogue& database, RoutingSettings& routingSettings) {
    routingSettings.bus_wait_time_ = database.rus().bus_wait_time();
    routingSettings.bus_velocity_ = database.rus().bus_velocity();
}

void desirialize(std::istream& in) {
    const json::Document rawRequests = json::Load(in);
    //DB
    const json::Node& serializationSettings = rawRequests.GetRoot().AsMap().at("serialization_settings");
    std::string fileName = serializationSettings.AsMap().at("file"s).AsString();
    std::ifstream inFile(fileName, std::ios::binary);
    proto::TransportCatalogue db;
    db.ParseFromIstream(&inFile);

    //TransportCatalogue
    database::TransportCatalogue catalogue;
    database_restore(db, catalogue);

    //Render settings
    rndr::renderSettings renderSettings;
    settings_restore(db, renderSettings);
    rndr::map_renderer renderer(catalogue, renderSettings);

    //Routing settings
    RoutingSettings routingSettings;
    settings_restore(db, routingSettings);
    database::transport_router finder(catalogue, routingSettings.bus_wait_time_, routingSettings.bus_velocity_);

    //Stat requests
    std::vector<request> statRequests = json_reader::parseStatRequests(rawRequests.GetRoot().AsMap().at("stat_requests"));

    //Answers in cout
    std::cout << json_reader::prepareAnswers(statRequests, catalogue, renderer, finder).Content();
}
