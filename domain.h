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

        Coordinates geo_tag_;
		std::string name_;
	};

    struct BusRout {
	public:
        BusRout() = default;
        BusRout(const BusRout& route);
        BusRout(BusRout&& route);

	bool cycled_ = 0;
	int stop_count_ = 0;
	int uniquestop_count_ = 0;
	double length_ = 0.0;
	double direct_Length_ = 0.0;
	double curve_ = 0.0;
	std::string number_ = "";
	std::vector<Stop*> da_way_;
	};

    struct RouteComparator {
        bool operator() (const BusRout* lhs, const BusRout* rhs) const {
	    return std::lexicographical_compare(
		lhs->number_.begin(), lhs->number_.end(),
		rhs->number_.begin(), rhs->number_.end()
	    );
        }
    };

    struct StopComparator {
        bool operator() (const Stop* lhs, const Stop* rhs) const {
            return std::lexicographical_compare(
                lhs->name_.begin(), lhs->name_.end(),
                rhs->name_.begin(), rhs->name_.end()
            );
        }
    };

    struct StopCompare {
        bool  operator()(const Stop* lhs, const Stop* rhs) const {
            return lhs->name_ == rhs->name_;
        }
    };

    struct RouteCompare {
        bool operator()(const BusRout* lhs, const BusRout* rhs) const {
            return lhs->number_ == rhs->number_;
        }
    };
}
