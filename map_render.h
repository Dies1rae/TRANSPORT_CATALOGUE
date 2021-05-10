#pragma once

#include "svg.h"
#include "transport_catalogue.h"
#include "geo.h"

#include <vector>
#include <string>
#include <array>
#include <math.h>
#include <unordered_map>
#include <algorithm>
#include <optional>

namespace rndr {
    inline const double EPSILON = 1e-6;
    struct renderSettings {
        renderSettings() = default;
        double width_ = 0.0;
        double height_ = 0.0;
        double padding_ = 0.0;
        double line_width_ = 0.0;
        double stop_radius_ = 0.0;
        int bus_label_font_size_ = 0;
        double underlayer_width_ = 0.0;
        int stop_label_font_size_ = 0;
        svg::Point bus_label_offset_{ 0 ,0 };
        svg::Point stop_label_offset_{ 0 ,0 };
        svg::Color underlayer_color_{};
        std::vector<svg::Color> color_palette_{};
    };

    class MapScaler {
    public:
        explicit inline MapScaler() = default;

        template <typename TStopIt>
        MapScaler(TStopIt points_begin, TStopIt points_end, double max_width,
            double max_height, double padding) : padding_(padding) {
            if (points_begin == points_end) {
                return;
            }

            const auto [left_it, right_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs->geo_tag_.lng < rhs->geo_tag_.lng;
            });

            min_lon_ = (*left_it)->geo_tag_.lng;
            const double max_lon = (*right_it)->geo_tag_.lng;

            const auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs->geo_tag_.lat < rhs->geo_tag_.lat;
            });
            const double min_lat = (*bottom_it)->geo_tag_.lat;
            max_lat_ = (*top_it)->geo_tag_.lat;

            std::optional<double> width_zoom;
            if (!this->IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            std::optional<double> height_zoom;
            if (!this->IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                zoom_coeff_ = std::min(width_zoom.value(), height_zoom.value());
            } else if (width_zoom) {
                zoom_coeff_ = width_zoom.value();
            } else if (height_zoom) {
                zoom_coeff_ = height_zoom.value();
            }
        }

        bool IsZero(double value) {
            return std::abs(value) < EPSILON;
        }

        svg::Point operator()(Coordinates coords) const {
            return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
        }
    private:
        double padding_ = 0.0;
        double min_lon_ = 0.0;
        double max_lat_ = 0.0;
        double zoom_coeff_ = 0.0;
    };

    class map_renderer {
    public:
        map_renderer(database::TransportCatalogue& catalogue, renderSettings& settings) : catalogue_(catalogue), settings_(settings) {}

        void Set_settings(const renderSettings& settings);

        void Set_catalogue(const database::TransportCatalogue& catalogue);

        void Test_all_settings();

        std::string render_map() const;
    private:
        database::TransportCatalogue& catalogue_;
        renderSettings& settings_;
    };

} //namespace rndr
