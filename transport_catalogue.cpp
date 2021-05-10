#include "transport_catalogue.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace database {
    void TransportCatalogue::rout_add(const BusRout& rout) {
        BusRout* tmp_rout = new BusRout(rout);
		this->base_routes_.push_back(tmp_rout);
	}

	void TransportCatalogue::rout_add(const std::string& rout_raw_data) {
        BusRout tmp_rout;
		size_t first_space = rout_raw_data.find(' ');
		size_t doubledotPos = rout_raw_data.find(':');
		std::string route_name = (rout_raw_data.substr(first_space + 1, doubledotPos - 4));
		const std::string delimetrs = "->";
		size_t word_begin = doubledotPos + 1;
		size_t word_end = doubledotPos + 1;
		std::vector<Stop*> stops;
		std::set<std::string> unique_stop;
		while (word_begin != std::string::npos) {
			word_begin = rout_raw_data.find_first_not_of(delimetrs, word_end);
			if (word_begin == std::string::npos) {
				break;
			}
			word_end = rout_raw_data.find_first_of(delimetrs, word_begin);
			if (word_end != std::string::npos) {
				tmp_rout.cycled_ = (rout_raw_data[word_end] == '-') && (word_end != std::string::npos);
			}
			std::string stopName = rout_raw_data.substr(word_begin + 1, word_end - word_begin - (word_end == std::string::npos ? 1 : 2));
			stops.push_back(this->stop_byname(stopName));
			unique_stop.insert(std::string(stopName));
		}

		tmp_rout.number_ = route_name;
		tmp_rout.da_way_ = std::move(stops);
		tmp_rout.uniquestop_count_ = unique_stop.size();
		if (tmp_rout.cycled_ == 0) {
			tmp_rout.stop_count_ = tmp_rout.da_way_.size();
		} else {
			tmp_rout.stop_count_ = (tmp_rout.da_way_.size() * 2) - 1;
		}

		for (size_t stp_pt = 0; stp_pt + 1 < tmp_rout.da_way_.size(); stp_pt++) {
            tmp_rout.direct_Length_ += ComputeDistance(tmp_rout.da_way_[stp_pt]->geo_tag_, tmp_rout.da_way_[stp_pt + 1]->geo_tag_);
		}
		if (tmp_rout.cycled_) {
			tmp_rout.direct_Length_ *= 2;
			for (int stp_pt = (int)tmp_rout.da_way_.size() - 1; stp_pt - 1 > -1; stp_pt--) {
				tmp_rout.length_ += this->get_Distance_byname(tmp_rout.da_way_[stp_pt], tmp_rout.da_way_[stp_pt - 1]);

			}
		}
		for (size_t stp_pt = 0; stp_pt + 1 < tmp_rout.da_way_.size(); stp_pt++) {
			tmp_rout.length_ += this->get_Distance_byname(tmp_rout.da_way_[stp_pt], tmp_rout.da_way_[stp_pt + 1]);
		}

		tmp_rout.curve_ = (tmp_rout.length_ / tmp_rout.direct_Length_) * 1.0;
		this->rout_add(tmp_rout);
	}

	void TransportCatalogue::rout_add(const std::string& rout_name, std::vector<elements::Stop*> stops, bool is_cycled) {
        BusRout tmp_rout;
		std::set<std::string> unique_stop;
		for (const auto& stop : stops) {
			unique_stop.insert(stop->name_);
		}
		
		tmp_rout.number_ = rout_name;
		tmp_rout.da_way_ = std::move(stops);
		tmp_rout.cycled_ = is_cycled;
		tmp_rout.uniquestop_count_ = unique_stop.size();

		if (tmp_rout.cycled_) {
			tmp_rout.stop_count_ = tmp_rout.da_way_.size();
		} else {
			tmp_rout.stop_count_ = (tmp_rout.da_way_.size() * 2) - 1;
		}

        for (int stp_pt = 0; stp_pt + 1 < tmp_rout.da_way_.size(); stp_pt++) {
            tmp_rout.direct_Length_ += ComputeDistance(tmp_rout.da_way_[stp_pt]->geo_tag_, tmp_rout.da_way_[stp_pt + 1]->geo_tag_);
		}
		if (!tmp_rout.cycled_) {
			tmp_rout.direct_Length_ *= 2;
			for (int stp_pt = (int)tmp_rout.da_way_.size() - 1; stp_pt - 1 > -1; stp_pt--) {
				tmp_rout.length_ += this->get_Distance_byname(tmp_rout.da_way_[stp_pt], tmp_rout.da_way_[stp_pt - 1]);

			}
		}
        for (int stp_pt = 0; stp_pt + 1 < tmp_rout.da_way_.size(); stp_pt++) {
			tmp_rout.length_ += this->get_Distance_byname(tmp_rout.da_way_[stp_pt], tmp_rout.da_way_[stp_pt + 1]);
		}

		
		tmp_rout.curve_ = (tmp_rout.length_ / tmp_rout.direct_Length_) * 1.0;
		this->rout_add(tmp_rout);
	}

	void TransportCatalogue::stop_add(const Stop& stop) {
		Stop* tmp_stop = new Stop(stop);
		this->base_stops_.push_back(tmp_stop);
	}

	void TransportCatalogue::stop_add(const std::string& stop_name, const std::string& lat, const std::string& lng) {
		Stop tmp_stop;
		tmp_stop.name_ = stop_name;
        tmp_stop.geo_tag_.lat = std::stod(std::string(lat).c_str());
        tmp_stop.geo_tag_.lng = std::stod(std::string(lng).c_str());
		this->stop_add(tmp_stop);
	}

	void TransportCatalogue::stop_add(const std::string& stop_name, const double lat, const double lng) {
		Stop tmp_stop;
		tmp_stop.name_ = stop_name;
        tmp_stop.geo_tag_.lat = lat;
        tmp_stop.geo_tag_.lng = lng;
		this->stop_add(tmp_stop);
	}

	void TransportCatalogue::test_output_bus(const std::string& bus_number) {
		for (const auto& buses : this->base_routes_) {
			if (buses->number_.find(bus_number) != std::string::npos) {
				std::cout << "Bus " << buses->number_ << ": ";
				for (auto stop : buses->da_way_) {
					std::cout << stop->name_ << " lat: ";
                    std::cout << std::setprecision(6) << stop->geo_tag_.lat << " lng: ";
                    std::cout << std::setprecision(6) << stop->geo_tag_.lng << " ";
				}
				std::cout << std::endl;
				break;
			}
		}
	}

	void TransportCatalogue::test_output_stop(const std::string& stop_name) {
		for (const auto& stops : this->base_stops_) {
			if (stops->name_.find(stop_name) != std::string::npos) {
				std::cout << "Stop " << stops->name_ << std::endl;
				std::cout << "lat ";
                std::cout << std::setprecision(6) << stops->geo_tag_.lat << " | lng ";
                std::cout << std::setprecision(6) << stops->geo_tag_.lng;
				std::cout << std::endl;
				break;
			}
		}
	}

	void TransportCatalogue::test_output_bus(const std::string_view bus_number) {
		for (const auto& buses : this->base_routes_) {
			if (buses->number_.find(bus_number) != std::string::npos) {
				std::cout << "Bus " << buses->number_ << ": ";
				for (auto stop : buses->da_way_) {
					std::cout << stop->name_ << " lat: ";
                    std::cout << std::setprecision(6) << stop->geo_tag_.lat << " lng: ";
                    std::cout << std::setprecision(6) << stop->geo_tag_.lng << " ";
				}
				std::cout << std::setprecision(6) << buses->length_ << " ";
				std::cout << std::endl;
				return;
			}
		}
		std::cout << "Bus "<< bus_number << " not found" << std::endl;
	}

	void TransportCatalogue::test_output_stop(const std::string_view stop_name) {
		for (const auto& stops : this->base_stops_) {
			if (stops->name_.find(stop_name) != std::string::npos) {
				std::cout << "Stop " << stops->name_ << std::endl;
				std::cout << "lat ";
                std::cout << std::setprecision(6) << stops->geo_tag_.lat << " | lng ";
                std::cout << std::setprecision(6) << stops->geo_tag_.lng;
				std::cout << std::endl;
				return;
			}
		}
		std::cout << "Stop "<< stop_name << " not found" << std::endl;
	}

	void TransportCatalogue::test_output_stop_distances() {
		for (const auto& [Stops, dist] : this->stops_to_distances_) {
			std::cout << "From: " << Stops.first->name_ << " To: " << Stops.second->name_ << ": " << dist << " meters" << std::endl;
		}
	}

    std::deque <BusRout*>& TransportCatalogue::get_base_routes() {
		return this->base_routes_;
	}

    std::set<BusRout*,elements::RouteComparator>& TransportCatalogue::get_base_routes_uniuqe() {
		return this->unique_rout_;
	}

	std::deque <Stop*>& TransportCatalogue::get_base_stops() {
		return this->base_stops_;
	}

    std::set<Stop*, elements::StopComparator>& TransportCatalogue::get_base_stops_uniuqe() {
		return this->unique_stop_;
	}

	std::unordered_map<std::pair<Stop*, Stop*>, double, TransportCatalogue::StpPHasher>& TransportCatalogue::get_distances() {
		return this->stops_to_distances_;
	}

	Stop* TransportCatalogue::stop_byname(const std::string_view name) {
		for(size_t stp_pt = 0; stp_pt < this->base_stops_.size(); stp_pt++){
			if (this->base_stops_[stp_pt]->name_ == name) {
				return this->base_stops_[stp_pt];
			}
		}
		return nullptr;
	}

    BusRout* TransportCatalogue::rout_byname(const std::string_view name) {
		for (size_t rt_pt = 0; rt_pt < this->base_routes_.size(); rt_pt++) {
			if (this->base_routes_[rt_pt]->number_ == name) {
				return this->base_routes_[rt_pt];
			}
		}
		return nullptr;
	}

	TransportCatalogue::RouteInfo TransportCatalogue::rout_info(const std::string_view bus_number) {
		TransportCatalogue::RouteInfo result;
		result.name_ = bus_number;
		if (this->rout_byname(bus_number) == nullptr) {
			return result;
		}

        BusRout* route = this->rout_byname(bus_number);
	
		result.stop_Count_ = route->stop_count_;
		result.unique_Stop_Count_ = route->uniquestop_count_;
		result.length_ = route->length_;
		result.curvature_ = route->curve_;
		result.direct_Length_ = route->direct_Length_;
		result.opt_ptr = true;
		return result;
	}

	TransportCatalogue::StopInfo TransportCatalogue::StopInfo(const std::string_view stop_name) {
		StopInfo result;
		result.name_ = stop_name;
		for (auto& [stop, routes] : this->stop_to_routes_) {
			if (stop->name_ == stop_name) {
				result.routes = this->stop_to_routes_.at(this->stop_byname(stop_name));
				result.opt_ptr = true;
				
			}
		} 
		return result;
	}

	void TransportCatalogue::stop_to_routes_build() {
		for (size_t stp_pt = 0; stp_pt < this->base_stops_.size(); stp_pt++) {
            std::set<BusRout*, RouteComparator> tmp_routes;
			for (size_t rt_pt = 0; rt_pt < this->base_routes_.size(); rt_pt++) {
				if (std::find(this->base_routes_[rt_pt]->da_way_.begin(), this->base_routes_[rt_pt]->da_way_.end(), this->base_stops_[stp_pt]) != this->base_routes_[rt_pt]->da_way_.end()) {
					tmp_routes.insert(this->base_routes_[rt_pt]);
					this->unique_stop_.insert(this->base_stops_[stp_pt]);
					
				}
			}
			this->stop_to_routes_[this->base_stops_[stp_pt]] = tmp_routes;
			this->unique_rout_.insert(tmp_routes.begin(), tmp_routes.end());
		}
	}

	void TransportCatalogue::setDistance(Stop* first,  Stop* second, const  double meters) {
		if (first != nullptr && second != nullptr) {
			this->stops_to_distances_[{ first, second } ] = meters;
		} else {
			throw std::invalid_argument("Invalid stop");
		}
	}

	void TransportCatalogue::setDistance(const std::string_view first, const std::string_view second, const double meters) {
		if (this->stop_byname(first) == nullptr || this->stop_byname(second) == nullptr) {
			throw std::invalid_argument("Invalid stop");
		}
		this->setDistance(this->stop_byname(first), this->stop_byname(second), meters);
	}

	int TransportCatalogue::get_Distance_byname(Stop* first, Stop* second) {
		if ((first != nullptr) && (second != nullptr)) {
			auto IT = this->stops_to_distances_.find({ first, second });

			if (IT != this->stops_to_distances_.end()) {
				return IT->second;
			}

			IT = this->stops_to_distances_.find({ second, first });
			if (IT != this->stops_to_distances_.end()) {
				return IT->second;
			}

			if (first == second) {
				return 0;
			}
		} else {
			throw std::invalid_argument("Invalid stops");
		}
		return 0;
	}

    std::unordered_map<Stop*, std::set<BusRout*, RouteComparator>, TransportCatalogue::StopHasher>& TransportCatalogue::get_stop_to_routes_() {
		return this->stop_to_routes_;
	}

	std::ostream& operator<<(std::ostream& out, const database::TransportCatalogue::RouteInfo& rout) {
		out << "Bus " << rout.name_ << ": ";
		if (rout.stop_Count_ != 0) {
			out << rout.stop_Count_ << " stops on route, ";
			out << rout.unique_Stop_Count_ << " unique stops, ";
			out << std::setprecision(6) << rout.length_ << " route length, " << rout.curvature_ << " curvature";
		} else {
			out << "not found";
		}
		return out;
	}

	std::ostream& operator<<(std::ostream& out, const database::TransportCatalogue::StopInfo& stop) {
		out << "Stop " << stop.name_ << ": ";
		if (stop.routes.size() == 0) {
			out << "no buses";
		} else {
			out << "buses";
            for (const BusRout* route : stop.routes) {
				out << " " << route->number_;
			}
		}
		return out;
	}
}
