#pragma once

#include "transport_catalogue.h"
#include "router.h"

#include <optional>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <ostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <memory>

namespace database {
    using namespace elements;

    struct RoutingSettings {
        double bus_wait_time_ = 0.0;
        double bus_velocity_ = 0.0;
    };

    struct TripTime {
        TripTime() = default;
        TripTime(int c, double t, double w) : stopCount(c), trip(t), wait(w) {}
        int stopCount = 0;
        double trip = 0.0;
        double wait = 0.0;
    };
    TripTime operator+ (const TripTime& lhs, const TripTime& rhs);
    bool operator> (const TripTime& lhs, const TripTime& rhs);
    bool operator< (const TripTime& lhs, const TripTime& rhs);

    struct TripItem {
        TripItem() = default;
        TripItem(Stop* f, Stop* t, BusRoute* r, TripTime trip_info) : from(f), to(t), route(r), Trip_item(trip_info) {}
        Stop* from = nullptr;
        Stop* to = nullptr;
        BusRoute* route = nullptr;
        TripTime Trip_item = {};
    };
    std::ostream& operator<<(std::ostream& out, const TripItem& item);
	
	class transport_router {
    public:
        transport_router(TransportCatalogue& catalogue, double bus_wait_time, double bus_velocity) : catalogue_(catalogue) {
            this->rout_settings_.bus_wait_time_ = bus_wait_time * 60.0;
            this->rout_settings_.bus_velocity_ = bus_velocity / 3.6;
            
            this->graph_ = new graph::DirectedWeightedGraph<TripTime>(this->catalogue_.get_base_stops().size());

            size_t ctr = 0;
            for (Stop* S : this->catalogue_.get_base_stops()) {
                this->graphVertexes_[S] = ctr++;
            }

            for (BusRoute* R : this->catalogue_.get_base_routes_uniuqe()) {
                distances_range(R);
                for (int s = 0; s + 1 < R->da_way_.size(); s++) {
                    for (int s1 = s + 1; s1 < R->da_way_.size(); s1++) {
                        addTripItem(R->da_way_[s], R->da_way_[s1], R, { std::abs(s - s1),  this->rout_settings_.bus_wait_time_, distanceBetween(s, s1) / this->rout_settings_.bus_velocity_ });
                        if (!R->cycled_) {
                            addTripItem(R->da_way_[s1], R->da_way_[s], R, { std::abs(s - s1),  this->rout_settings_.bus_wait_time_, distanceBetween(s1, s) / this->rout_settings_.bus_velocity_ });
                        }
                    }
                }
            }
            this->searching_edges_ = new graph::Router<TripTime>(*this->graph_);
        }

        void distances_range(const BusRoute* rout);

        double distanceBetween(const size_t from, const size_t to);

        void addTripItem(Stop* from, Stop* to, BusRoute* route, TripTime&& trip_info);

        std::optional<std::vector <TripItem>> findRoute(const std::string_view from, const std::string_view to);
	private:
        std::unordered_map<Stop*, size_t> graphVertexes_;
        std::vector<TripItem> graphEdges_;
        graph::DirectedWeightedGraph<TripTime>* graph_;
        graph::Router<TripTime>* searching_edges_;
        RoutingSettings rout_settings_;
        TransportCatalogue& catalogue_;
        std::vector<double> direct_dist_;
        std::vector<double> reverse_dist_;
	};
} //database
