#pragma once

#include "map_render.h"
#include "transport_catalogue.h"
#include "json_builder.h"
#include "transport_router.h"


#include <iostream>
#include <string>

using namespace std::literals;
using namespace database;

svg::Color nodeToColor(const json::Node& node);

enum class request_type {
    REQUEST_STOP, REQUEST_BUS, REQUEST_RENDER, REQUEST_ROUTE
};

struct RoutingSettings {
    double bus_wait_time_ = 0.0;
    double bus_velocity_ = 0.0;
};

struct request {
    int id;
    request_type type;
    std::string name;
    std::string from;
    std::string to;
};

struct stopAnswer {
    int id = 0;
    TransportCatalogue::Stop_info answer;
    std::string error = "";
};

json::Node makeStopAnswer(const int request_id, const TransportCatalogue::Stop_info& data);
json::Node makeRouteAnswer(const int request_id, const TransportCatalogue::Route_info& data);
json::Node makeRouteTimeAnswer(const int request_id, const std::vector <TripItem>& trip);
json::Node makeRanderAnswer(const int request_id, const std::string& map_render_data);
svg::Color parse_color(const json::Node& node);

class json_reader{
public:
    json_reader(TransportCatalogue& catalogue):catalogue_(catalogue) {}
    void process_queries(std::istream& in = std::cin, std::ostream& out = std::cout);
    static rndr::renderSettings parseRenderSettings(const json::Node& node);
    static RoutingSettings parseRoutingSettings(const json::Node& node);
    static std::vector<request> parseStatRequests(const json::Node& node);
    static void fillDataBase(database::TransportCatalogue& catalogue, const std::vector<const json::Node*>& stopNodes, const std::vector<const json::Node*>& routeNodes);
    static json::Node prepareAnswers(std::vector<request>& requests, database::TransportCatalogue& catalogue, rndr::map_renderer& renderer, database::transport_router& finder);

private:
    TransportCatalogue& catalogue_;
};
