#include "transport_router.h"

namespace database {
    using namespace elements;

    TripTime operator+ (const TripTime& lhs, const TripTime& rhs) {
        return { lhs.stopCount + rhs.stopCount, lhs.trip + rhs.trip, lhs.wait + rhs.wait };
    }
    bool operator> (const TripTime& lhs, const TripTime& rhs) {
        return (lhs.trip + lhs.wait > rhs.trip + rhs.wait);
    }
    bool operator< (const TripTime& lhs, const TripTime& rhs) {
        return (lhs.trip + lhs.wait < rhs.trip + rhs.wait);
    }

    std::ostream& operator<<(std::ostream& out, const TripItem& item) {
        out << item.from->name_ << " -> " << item.to->name_
            << " by bus " << item.route->number_ << " : " << item.Trip_item.stopCount << " stops, "
            << item.Trip_item.wait << " min wait time "
            << item.Trip_item.trip << " min trip time "
            << item.Trip_item.trip + item.Trip_item.wait << " min total time";
        return out;
    }

    void transport_router::distances_range(const BusRout* rout) {
        this->direct_dist_.resize(rout->da_way_.size());
        this->reverse_dist_.resize(rout->da_way_.size());
        double directDistanceSum = 0.0;
        double reverseDistanceSum = 0.0;
        for (size_t ptr = 1; ptr < rout->da_way_.size(); ptr++) {
            directDistanceSum += this->catalogue_.get_Distance_byname(rout->da_way_[ptr - 1], rout->da_way_[ptr]);
            this->direct_dist_[ptr] = directDistanceSum;
            reverseDistanceSum += this->catalogue_.get_Distance_byname(rout->da_way_[ptr], rout->da_way_[ptr - 1]);
            this->reverse_dist_[ptr] = reverseDistanceSum;
        }
    }

    double transport_router::distanceBetween(const size_t from, const size_t to) {
        if (from < to) {
            return this->direct_dist_[to] - this->direct_dist_[from];
        } else {
            return -(this->reverse_dist_[to] - this->reverse_dist_[from]);
        }
    }

    void transport_router::addTripItem(Stop* from, Stop* to, BusRout* route, TripTime&& trip_info) {
        TripItem item{ from, to, route, std::move(trip_info) };
        int id = this->graph_->AddEdge(graph::Edge<TripTime>{this->graphVertexes_[item.from], this->graphVertexes_[item.to], item.Trip_item});
        this->graphEdges_.push_back(std::move(item));
        if (id != (int)graphEdges_.size() - 1) {
            throw std::exception();
        }
    }

    std::optional<std::vector <TripItem>> transport_router::findRoute(const std::string_view from, const std::string_view to) {
        Stop* src = this->catalogue_.stop_byname(from);
        Stop* dst = this->catalogue_.stop_byname(to);
        if (src == nullptr || dst == nullptr) {
            return std::nullopt;
        }

        std::vector <TripItem> res;
        if (src == dst) {
            return res;
        }

        graph::VertexId src_vertex = this->graphVertexes_.at(src);
        graph::VertexId dst_vertex = this->graphVertexes_.at(dst);

        std::unique_ptr<std::vector<size_t>>  res_tmp = this->searching_edges_->BuildRoute(src_vertex, dst_vertex);
        if (res_tmp != nullptr) {
            res.reserve(res_tmp->size());
            for (auto e = res_tmp->begin(); e != res_tmp->end(); e++) {
                res.push_back(std::move(this->graphEdges_.at(*e)));
            }
            return res;
        }

        return std::nullopt;
    }
} //database
