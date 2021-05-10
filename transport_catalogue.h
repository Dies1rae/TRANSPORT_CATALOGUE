#pragma once

#include "geo.h"
#include "domain.h"

#include <string>
#include <deque>
#include <unordered_set>
#include <math.h>
#include <list>
#include <algorithm>
#include <unordered_map>
#include <set>

namespace database{
	using namespace elements;
	class TransportCatalogue {
	public:
		inline static constexpr size_t hasher_salt = 58;
		struct Stop_Hasher {
			int operator()(const Stop* stop_name) const {
				int res = 0;
				int ptr = 1;
				 for (char ch : stop_name->name_) {
					 res += ch - 'A';
					 ptr++;
				 }
				 res += hasher_salt* ptr;
				 return res;
			 }
		};

		struct Route_Hasher {
			int operator()(const BusRout* Bus_num) const {
				int res = 0;
				int ptr = 1;
				for (char ch : Bus_num->number_) {
					res += ch - 'A';
					ptr++;
				}
				res += hasher_salt * ptr;
				return res;
			}
		};

		struct StpPHasher {
			int operator()(const std::pair<Stop*, Stop*>& stop_pair) const {
				int res = 0;
				int ptr = 1;
				for (char ch : stop_pair.first->name_) {
					res += ch - 'A';
					ptr++;
				}
				for (char ch : stop_pair.second->name_) {
					res += ch - 'A';
					ptr++;
				}
				res += hasher_salt * ptr;
				return res;
			}
		};

		struct Route_info {
		public:
			Route_info() = default;
			bool opt_ptr = false;
			int stop_Count_ = 0;
			int unique_Stop_Count_ = 0;
			double length_ = 0.0;
			double direct_Length_ = 0.0;
			double curvature_ = 0.0;
			std::string_view name_ = "";
		};

		struct Stop_info {
		public:
			Stop_info() = default;
			bool opt_ptr = false;
			std::string_view name_ = "";
			std::set<BusRout*, RouteComparator> routes;
		};

		explicit TransportCatalogue() = default;
		~TransportCatalogue() {
			this->stop_to_routes_.clear();
			this->stops_to_distances_.clear();
			this->unique_stop_.clear();
			this->unique_rout_.clear();
			for (auto S : this->base_stops_) {
				delete S;
			}
			for (auto R : this->base_routes_) {
				delete R;
			}
			this->base_routes_.clear();
			this->base_stops_.clear();
		}
	
		void stop_add(const std::string& stop_name, const std::string& lat, const std::string& lng);
		void stop_add(const std::string& stop_name, const double lat, const double lng);
		void stop_add(const Stop& stop);
		void rout_add(const std::string& rout_raw_data);
		void rout_add(const BusRout& rout);
		void rout_add(const std::string& rout_name, std::vector<elements::Stop*> stops, bool is_cycled);
		void stop_to_routes_build();
		Stop* stop_byname(const std::string_view name);
		BusRout* rout_byname(const std::string_view name);

		void test_output_bus(const std::string& bus_number);
		void test_output_stop(const std::string& bus_number);
		void test_output_bus(const std::string_view bus_number);
		void test_output_stop(const std::string_view bus_number);
		void test_output_stop_distances();

		Route_info rout_info(const std::string_view bus_number);
		Stop_info stop_info(const std::string_view stop_name);

		void setDistance(Stop* first, Stop* second, const double meters);
		void setDistance(const std::string_view first,	const std::string_view second, const double meters);
		int get_Distance_byname(Stop* first, Stop* second);

		std::deque <BusRout*>& get_base_routes();
		std::set<BusRout*, elements::RouteComparator>& get_base_routes_uniuqe();
		std::set<Stop*, elements::StopComparator>& get_base_stops_uniuqe();
		std::deque <Stop*>& get_base_stops();
		std::unordered_map<std::pair<Stop*, Stop*>, double, StpPHasher>& get_distances();
		std::unordered_map<Stop*, std::set<BusRout*, RouteComparator>, Stop_Hasher>& get_stop_to_routes_();
	private:
		std::deque <BusRout*> base_routes_;
		std::deque <Stop*> base_stops_;
		std::unordered_map<Stop*, std::set<BusRout*, RouteComparator>, Stop_Hasher> stop_to_routes_;
		std::unordered_map<std::pair<Stop*, Stop*>, double, StpPHasher> stops_to_distances_;
		std::set<Stop*, elements::StopComparator> unique_stop_;
		std::set<BusRout*, elements::RouteComparator> unique_rout_;
	};

	std::ostream& operator<<(std::ostream& out, const TransportCatalogue::Route_info& rout);
	std::ostream& operator<<(std::ostream& out, const TransportCatalogue::Stop_info& stop);
}
