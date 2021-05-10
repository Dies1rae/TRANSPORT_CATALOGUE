#include "json_reader.h"

#include <iomanip>
#include <vector>
#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
#include <iterator>

json::Node makeStopAnswer(const int request_id, const TransportCatalogue::Stop_info& data) {
    json::Builder b_;
    b_.StartArray();
    for (elements::BusRout* r : data.routes) {
        b_.Value(r->number_);
    }
    json::Node tmp = b_.EndArray().Build();
    return json::Builder{}.StartDict().Key("request_id"s).Value(request_id).Key("buses").Value(tmp.AsArray()).EndDict().Build();
}

json::Node makeRouteAnswer(const int request_id, const TransportCatalogue::Route_info& data) {
    return json::Builder{}.StartDict().Key("request_id"s).Value(request_id).Key("route_length"s).Value(data.length_)
        .Key("stop_count"s).Value(data.stop_Count_).Key("unique_stop_count"s).Value(data.unique_Stop_Count_)
        .Key("curvature"s).Value(data.curvature_).EndDict().Build();
}

json::Node makeRouteTimeAnswer(const int request_id, const std::vector <TripItem>& trip) {
    json::Builder builder;
    builder.StartArray();
    double total_time = 0.0;
    for (auto iter = trip.begin(); iter != trip.end(); iter++) {
        total_time += (iter->Trip_item.wait + iter->Trip_item.trip);
        builder.StartDict()
            .Key("type"s).Value("Wait"s)
            .Key("stop_name"s).Value(iter->from->name_)
            .Key("time"s).Value(iter->Trip_item.trip / 60)
            .EndDict()

            .StartDict()
            .Key("type"s).Value("Bus"s)
            .Key("bus"s).Value(iter->route->number_)
            .Key("span_count"s).Value(iter->Trip_item.stopCount)
            .Key("time"s).Value(iter->Trip_item.wait / 60)
            .EndDict();
    }
    json::Node x = builder.EndArray().Build();

    return json::Builder{}
        .StartDict()
        .Key("request_id"s).Value(request_id)
        .Key("total_time"s).Value(total_time / 60)
        .Key("items"s).Value(x.AsArray())
        .EndDict().Build();
}

json::Node makeRanderAnswer(const int request_id, const std::string& map_render_data) {
    return json::Builder{}.StartDict().Key("map"s).Value(map_render_data).Key("request_id"s).Value(request_id).EndDict().Build();
}

svg::Color parse_color(const json::Node& node) {
    if (node.IsString()) {
        svg::Color tmp_string_col{ node.AsString() };
        return tmp_string_col;
    }
    if (node.IsArray()) {
        if (node.AsArray().size() == 4) {

            svg::Rgba rgba_tmp{ (uint8_t)node.AsArray()[0].AsInt(), (uint8_t)node.AsArray()[1].AsInt(), (uint8_t)node.AsArray()[2].AsInt(), node.AsArray()[3].AsDouble() };
            return rgba_tmp;
        }
        if (node.AsArray().size() == 3) {
            svg::Rgb rgb_tmp{ (uint8_t)node.AsArray()[0].AsInt(), (uint8_t)node.AsArray()[1].AsInt(), (uint8_t)node.AsArray()[2].AsInt() };
            return rgb_tmp;
        }
    } else {
        throw  std::exception();
    }
    return svg::Color{};
}

void json_reader::process_queries(std::istream& in, std::ostream& out) {
    using namespace json;
    const Document raw_requests = json::Load(in);

    const Node base_requests = raw_requests.GetRoot().AsMap().at("base_requests");
    std::vector<Node> stop_nodes;
    std::vector<Node> routs_nodes;

    for (const Node& n : base_requests.AsArray()) {
        if (n.AsMap().at("type"s).AsString() == "Stop"s) {
            stop_nodes.push_back(n);
            continue;
        }
        if (n.AsMap().at("type"s).AsString() == "Bus"s) {
            routs_nodes.push_back(n);
            continue;
        }
        throw std::exception();
    }

    struct distance {
        std::string from;
        std::string to;
        double meters;
    };
    std::vector<distance> distances;

    //BD info!!
    // Stops
    for (const Node& n : stop_nodes) {
        std::string name = n.AsMap().at("name"s).AsString();
        double latitude = n.AsMap().at("latitude"s).AsDouble();
        double longitude = n.AsMap().at("longitude"s).AsDouble();
        this->catalogue_.stop_add(name, latitude, longitude);
        for (const auto& [first, second] : n.AsMap().at("road_distances"s).AsMap()) {
            distances.push_back({ name, first, second.AsDouble() });
        }
    }

    // Distances
    for (const distance& ptr : distances) {
        this->catalogue_.setDistance(ptr.from, ptr.to, ptr.meters);
    }

    // Routes
    for (const Node& n : routs_nodes) {
        std::string name = n.AsMap().at("name"s).AsString();
        bool isCycled = n.AsMap().at("is_roundtrip"s).AsBool();
        std::vector<elements::Stop*> stops;
        for (const Node& ptr : n.AsMap().at("stops"s).AsArray()) {
            stops.push_back(this->catalogue_.stop_byname(ptr.AsString()));
        }

        this->catalogue_.rout_add(name, std::move(stops), isCycled);
    }
    this->catalogue_.stop_to_routes_build();

    //RENDER SETTINGS!!!
    rndr::renderSettings Render_Settings;
    if (raw_requests.GetRoot().AsMap().find("render_settings") != raw_requests.GetRoot().AsMap().end()) {
        const Dict& render_requests = raw_requests.GetRoot().AsMap().at("render_settings").AsMap();
        Render_Settings.width_ = render_requests.at("width"s).AsDouble();
        Render_Settings.height_ = render_requests.at("height"s).AsDouble();
        Render_Settings.padding_ = render_requests.at("padding"s).AsDouble();
        Render_Settings.stop_radius_ = render_requests.at("stop_radius"s).AsDouble();
        Render_Settings.line_width_ = render_requests.at("line_width"s).AsDouble();
        Render_Settings.bus_label_font_size_ = render_requests.at("bus_label_font_size"s).AsInt();
        Render_Settings.underlayer_width_ = render_requests.at("underlayer_width"s).AsDouble();
        Render_Settings.stop_label_font_size_ = render_requests.at("stop_label_font_size"s).AsInt();
        Render_Settings.bus_label_offset_.x = render_requests.at("bus_label_offset"s).AsArray()[0].AsDouble();
        Render_Settings.bus_label_offset_.y = render_requests.at("bus_label_offset"s).AsArray()[1].AsDouble();
        Render_Settings.stop_label_offset_.x = render_requests.at("stop_label_offset"s).AsArray()[0].AsDouble();
        Render_Settings.stop_label_offset_.y = render_requests.at("stop_label_offset"s).AsArray()[1].AsDouble();
        Render_Settings.underlayer_color_ = parse_color(render_requests.at("underlayer_color"s));
        for (const auto& el : render_requests.at("color_palette"s).AsArray()) {
            Render_Settings.color_palette_.push_back(parse_color(el));
        }
    }

    //ROUTING SETTINGS!!!
    const Dict& route_requests = raw_requests.GetRoot().AsMap().at("routing_settings").AsMap();
    transport_router graph_finder(this->catalogue_, route_requests.at("bus_wait_time"s).AsDouble(), route_requests.at("bus_velocity"s).AsDouble());

    //Request!!
    const json::Node stat_requests = raw_requests.GetRoot().AsMap().at("stat_requests");
    std::vector<request> pure_requests;
    for (const Node& n : stat_requests.AsArray()) {
        request r;
        r.id = n.AsMap().at("id"s).AsInt();
        if (n.AsMap().at("type"s).AsString() == "Stop") {
            r.name = n.AsMap().at("name"s).AsString();
            r.type = request_type::REQUEST_STOP;
        } else if (n.AsMap().at("type"s).AsString() == "Bus") {
            r.name = n.AsMap().at("name"s).AsString();
            r.type = request_type::REQUEST_BUS;
        } else if (n.AsMap().at("type"s).AsString() == "Map") {
            r.type = request_type::REQUEST_RENDER;
        } else if (n.AsMap().at("type"s).AsString() == "Route") {
            r.name = n.AsMap().at("from"s).AsString() + "|" + n.AsMap().at("to"s).AsString();
            r.type = request_type::REQUEST_ROUTE;

        } else {
            throw json::ParsingError("Stat request type parsing error"s);
        }
        pure_requests.push_back(r);
    }

    // Process requests

    json::Array ans_part(pure_requests.size());
    size_t req = 0;

    for (const request& r : pure_requests) {
        switch (r.type) {
        case request_type::REQUEST_STOP: {
            if (auto stop_info = this->catalogue_.stop_info(r.name); stop_info.opt_ptr) {
                ans_part[req] = makeStopAnswer(r.id, stop_info).AsMap();
            } else {
                ans_part[req] = Dict{ {"request_id"s,r.id},{"error_message"s, "not found"s} };
            }
        } break;
        case request_type::REQUEST_BUS: {
            if (auto route_info = TransportCatalogue::Route_info(std::move(this->catalogue_.rout_info(r.name))); route_info.opt_ptr) {
                ans_part[req] = makeRouteAnswer(r.id, route_info).AsMap();
            } else {
                ans_part[req] = Dict{ {"request_id"s,r.id},{"error_message"s, "not found"s} };
            }
        } break;
        case request_type::REQUEST_ROUTE: {
            if (auto graph_router = graph_finder.findRoute(r.name.substr(0, r.name.find_first_of("|")), r.name.substr(r.name.find_first_of("|") + 1, r.name.size())); graph_router != std::nullopt) {
                ans_part[req] = makeRouteTimeAnswer(r.id, graph_router.value()).AsMap();
            } else {
                ans_part[req] = Dict{ {"request_id"s,r.id},{"error_message"s, "not found"s} };
            }

        } break;
        case request_type::REQUEST_RENDER: {
            rndr::map_renderer rendering(this->catalogue_, Render_Settings);
            ans_part[req] = json::Builder{}.StartDict().Key("request_id"s).Value(r.id).Key("map"s).Value(rendering.render_map()).EndDict().Build().AsMap();
        } break;
        default:
            throw std::exception();
        }
        req++;
    }
    out << json::Node(ans_part).Content();
}

rndr::renderSettings json_reader::parseRenderSettings(const json::Node& node) {
    const json::Dict& settingsDict = node.AsMap();
    rndr::renderSettings renderSettings;
    renderSettings.width_ = settingsDict.at("width"s).AsDouble();
    renderSettings.height_ = settingsDict.at("height"s).AsDouble();

    renderSettings.padding_ = settingsDict.at("padding"s).AsDouble();
    renderSettings.line_width_ = settingsDict.at("line_width"s).AsDouble();
    renderSettings.stop_radius_ = settingsDict.at("stop_radius"s).AsDouble();

    renderSettings.bus_label_font_size_ = settingsDict.at("bus_label_font_size"s).AsInt();
    json::Array rawBusLabelOffset = settingsDict.at("bus_label_offset"s).AsArray();
    renderSettings.bus_label_offset_.x = rawBusLabelOffset[0].AsDouble();
    renderSettings.bus_label_offset_.y = rawBusLabelOffset[1].AsDouble();


    renderSettings.stop_label_font_size_ = settingsDict.at("stop_label_font_size"s).AsInt();
    json::Array rawStopLabelOffset = settingsDict.at("stop_label_offset"s).AsArray();
    renderSettings.stop_label_offset_.x = rawStopLabelOffset[0].AsDouble();
    renderSettings.stop_label_offset_.y = rawStopLabelOffset[1].AsDouble();


    renderSettings.underlayer_color_ = nodeToColor(settingsDict.at("underlayer_color"s));
    renderSettings.underlayer_width_ = settingsDict.at("underlayer_width"s).AsDouble();

    for (const json::Node& n : settingsDict.at("color_palette"s).AsArray()) {
        renderSettings.color_palette_.push_back(nodeToColor(n));
    }
    return renderSettings;
}

RoutingSettings json_reader::parseRoutingSettings(const json::Node& node) {
    const json::Dict& settingsDict = node.AsMap();
    RoutingSettings routingSettings;
    routingSettings.bus_wait_time_ = settingsDict.at("bus_wait_time"s).AsInt();
    routingSettings.bus_velocity_ = settingsDict.at("bus_velocity"s).AsDouble();
    return routingSettings;
}

std::vector<request> json_reader::parseStatRequests(const json::Node& node) {
    std::vector<request> result;
    for (const json::Node& n : node.AsArray()) {
        request r;
        r.id = n.AsMap().at("id"s).AsInt();
        if (n.AsMap().at("type"s).AsString() == "Stop") {
            r.type = request_type::REQUEST_STOP;
            r.name = n.AsMap().at("name"s).AsString();
        }
        else if (n.AsMap().at("type"s).AsString() == "Bus") {
            r.type = request_type::REQUEST_BUS;
            r.name = n.AsMap().at("name"s).AsString();
        }
        else if (n.AsMap().at("type"s).AsString() == "Map") {
            r.type = request_type::REQUEST_RENDER;
        }
        else if (n.AsMap().at("type"s).AsString() == "Route") {
            r.type = request_type::REQUEST_ROUTE;
            r.from = n.AsMap().at("from"s).AsString();
            r.to = n.AsMap().at("to"s).AsString();
        }
        else {
            throw json::ParsingError("Stat request type parsing error"s);
        }
        result.push_back(r);
    }
    return result;
}

void json_reader::fillDataBase(database::TransportCatalogue& catalogue, const std::vector<const json::Node*>& stopNodes, const std::vector<const json::Node*>& routeNodes) {
    using namespace json;
    struct Distance {
        std::string from;
        std::string to;
        int meters;
    };
    std::vector<Distance> distances;

    for (const Node* node : stopNodes) {
        std::string name = node->AsMap().at("name"s).AsString();
        double latitude = node->AsMap().at("latitude"s).AsDouble();
        double longitude = node->AsMap().at("longitude"s).AsDouble();
        catalogue.stop_add(name, latitude, longitude );
        for (const auto& d : node->AsMap().at("road_distances"s).AsMap()) {
            distances.push_back({ name,d.first,d.second.AsInt() });
        }
    }

    for (const Distance& distance : distances) {
        catalogue.setDistance(distance.from, distance.to, distance.meters);
    }

    for (const Node* n : routeNodes) {
        std::string name = n->AsMap().at("name"s).AsString();
        bool isCycled = n->AsMap().at("is_roundtrip"s).AsBool();
        std::vector<database::Stop*> stops;
        for (const Node& n : n->AsMap().at("stops"s).AsArray()) {
            if (auto stop = catalogue.stop_byname(n.AsString()); stop) {
                stops.push_back(stop);
            } else {
                throw std::invalid_argument("Invalid stop: "s + n.AsString());
            }
        }
        catalogue.rout_add(name, std::move(stops), isCycled);
    }
}

json::Node json_reader::prepareAnswers(std::vector<request>& requests, database::TransportCatalogue& catalogue, rndr::map_renderer& renderer, database::transport_router& finder) {
    json::Array answers(requests.size());
    size_t req = 0;
    for (const request& r : requests) {
        switch (r.type) {
        case request_type::REQUEST_STOP: {
            if (auto stop_info = catalogue.stop_info(r.name); stop_info.opt_ptr) {
                answers[req] = makeStopAnswer(r.id, stop_info).AsMap();
            } else {
                answers[req] = json::Dict{ {"request_id"s,r.id},{"error_message"s, "not found"s} };
            }
        } break;
        case request_type::REQUEST_BUS: {
            if (auto route_info = TransportCatalogue::Route_info(std::move(catalogue.rout_info(r.name))); route_info.opt_ptr) {
                answers[req] = makeRouteAnswer(r.id, route_info).AsMap();
            } else {
                answers[req] = json::Dict{ {"request_id"s,r.id},{"error_message"s, "not found"s} };
            }
        } break;
        case request_type::REQUEST_ROUTE: {
            if (auto graph_router = finder.findRoute(r.name.substr(0, r.name.find_first_of("|")), r.name.substr(r.name.find_first_of("|") + 1, r.name.size())); graph_router != std::nullopt) {
                answers[req] = makeRouteTimeAnswer(r.id, graph_router.value()).AsMap();
            } else {
                answers[req] = json::Dict{ {"request_id"s,r.id},{"error_message"s, "not found"s} };
            }

        } break;
        case request_type::REQUEST_RENDER: {
            answers[req] = json::Builder{}.StartDict().Key("request_id"s).Value(r.id).Key("map"s).Value(renderer.render_map()).EndDict().Build().AsMap();
        } break;
        default:
            throw std::exception();
        }
        req++;
    }
    return answers;
}

svg::Color nodeToColor(const json::Node& node) {
    if (node.IsString()) {
        return svg::Color(node.AsString());
    }
    if (node.IsArray()) {
        const json::Array& color = node.AsArray();
        if (color.size() == 3) {
            return svg::Color(
                svg::Rgb(color[0].AsInt(), color[1].AsInt(), color[2].AsInt()));
        }
        if (color.size() == 4) {
            return svg::Color(svg::Rgba(color[0].AsInt(), color[1].AsInt(), color[2].AsInt(), color[3].AsDouble()));
        }
    }
    throw std::exception();
}
