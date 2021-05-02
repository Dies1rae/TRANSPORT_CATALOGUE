#pragma once

#include <string>
#include <vector>
#include <set>
#include "geo.h"

namespace elements {
	struct Stop {
	public:
		Stop();
		Stop(const std::string_view name, const double latitude, const double longitude);
        Stop(const std::string name, Coordinates coords);
		Stop(const Stop& stop);
		Stop(Stop&& stop);

		Coordinates geo_tag;
		std::string name_;
	};

	struct Bus_rout {
	public:
		Bus_rout() = default;
		Bus_rout(const Bus_rout& route);
		Bus_rout(Bus_rout&& route);

		bool cycled_ = 0;
		int stop_count_ = 0;
		int uniquestop_count_ = 0;
		double length_ = 0.0;
		double direct_Length_ = 0.0;
		double curve_ = 0.0;
		std::string number_ = "";
		std::vector<Stop*> da_way_;
	};

	struct Route_comparator {
		bool operator() (const Bus_rout* lhs, const Bus_rout* rhs) const {
			return std::lexicographical_compare(
				lhs->number_.begin(), lhs->number_.end(),
				rhs->number_.begin(), rhs->number_.end()
			);
		}
	};

	struct Stop_comparator {
		bool operator() (const Stop* lhs, const Stop* rhs) const {
			return std::lexicographical_compare(
				lhs->name_.begin(), lhs->name_.end(),
				rhs->name_.begin(), rhs->name_.end()
			);
		}
	};

	struct Stop_compare {
		bool  operator()(const Stop* lhs, const Stop* rhs) const {
			return lhs->name_ == rhs->name_;
		}
	};

	struct Route_compare {
		bool operator()(const Bus_rout* lhs, const Bus_rout* rhs) const {
			return lhs->number_ == rhs->number_;
		}
	};
}
