#include "domain.h"

#include <iostream>
#include <iomanip>

namespace elements {
    BusRoute::BusRoute(const BusRoute& route) {
	    this->cycled_ = route.cycled_;
	    this->stop_count_ = route.stop_count_;
	    this->uniquestop_count_ = route.uniquestop_count_;
	    this->length_ = route.length_;
	    this->direct_Length_ = route.direct_Length_;
	    this->curve_ = route.curve_;
	    this->number_ = route.number_;
	    this->da_way_ = route.da_way_;
    }
    BusRoute::BusRoute(BusRoute&& route) {
	    this->cycled_ = std::move(route.cycled_);
	    this->stop_count_ = std::move(route.stop_count_);
	    this->uniquestop_count_ = std::move(route.uniquestop_count_);
	    this->length_ = std::move(route.length_);
	    this->direct_Length_ = std::move(route.direct_Length_);
	    this->curve_ = std::move(route.curve_);
	    this->number_ = std::move(route.number_);
	    this->da_way_ = std::move(route.da_way_);
    }

    Stop::Stop() {
        this->name_ = "";
        this->geo_tag_.lat = 0;
        this->geo_tag_.lng = 0;
    }
    Stop::Stop(const std::string_view name, const double latitude, const double longitude) : name_(name) {
        this->geo_tag_.lat = latitude;
        this->geo_tag_.lng = longitude;
    }
    Stop::Stop(const std::string name, Coordinates coords) {
        this->name_ = name;
        this->geo_tag_ = coords;
    }
    Stop::Stop(const Stop& stop) {
        this->name_ = stop.name_;
        this->geo_tag_ = stop.geo_tag_;
    }
    Stop::Stop(Stop&& stop) {
        this->name_ = std::move(stop.name_);
        this->geo_tag_ = std::move(stop.geo_tag_);
    }
}
