#include "map_render.h"

#include <set>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace rndr {
	std::string map_renderer::render_map() const {
        using namespace std::literals;

        std::set<elements::BusRoute*, elements::RouteComparator> routes_to_draw = this->catalogue_.get_base_routes_uniuqe();
        std::set<elements::Stop*, elements::StopComparator> stops_to_draw = this->catalogue_.get_base_stops_uniuqe();
        
        MapScaler scaler(stops_to_draw.begin(), stops_to_draw.end(), this->settings_.width_, this->settings_.height_, this->settings_.padding_);
        svg::Document svg_document;

        size_t ptr_clr = 0;
        for (const auto& r : routes_to_draw) {
            if (r->uniquestop_count_ < 2) {
                continue;
            }
            svg::Polyline route_line = svg::Polyline()
                .SetStrokeColor(this->settings_.color_palette_[ptr_clr])
                .SetStrokeWidth(this->settings_.line_width_)
                .SetFillColor({})
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            for (elements::Stop* s : r->da_way_) {
                route_line.AddPoint(scaler(s->geo_tag_));
            }
            if (!r->cycled_) {
                for (int el = r->da_way_.size() - 2; el > -1; el --) {
                    route_line.AddPoint(scaler(r->da_way_[el]->geo_tag_));
                }
            }
            svg_document.Add(route_line);
            
            ptr_clr++;
            if (ptr_clr == this->settings_.color_palette_.size()) {
                ptr_clr = 0;
            }
        }
        
        ptr_clr = 0;
        for (const auto& r : routes_to_draw) {
            std::vector<svg::Text> routs_nums;
            if (r->cycled_) {
                svg::Text routs_num_first = svg::Text()
                    .SetFillColor(this->settings_.color_palette_[ptr_clr])
                    .SetFontWeight("bold")
                    .SetPosition(scaler(r->da_way_[0]->geo_tag_))
                    .SetOffset(this->settings_.bus_label_offset_)
                    .SetFontSize(this->settings_.bus_label_font_size_)
                    .SetFontFamily("Verdana")
                    .SetData(r->number_);
                svg::Text routs_num_back = svg::Text()
                    .SetFillColor(this->settings_.underlayer_color_)
                    .SetStrokeColor(this->settings_.underlayer_color_)
                    .SetStrokeWidth(this->settings_.underlayer_width_)
                    .SetFontWeight("bold")
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                    .SetPosition(scaler(r->da_way_[0]->geo_tag_))
                    .SetOffset(this->settings_.bus_label_offset_)
                    .SetFontSize(this->settings_.bus_label_font_size_)
                    .SetFontFamily("Verdana")
                    .SetData(r->number_);
                routs_nums.push_back(routs_num_back);
                routs_nums.push_back(routs_num_first);
            } else {
                svg::Text routs_num_first = svg::Text()
                    .SetFillColor(this->settings_.color_palette_[ptr_clr])
                    .SetFontWeight("bold")
                    .SetPosition(scaler(r->da_way_[0]->geo_tag_))
                    .SetOffset(this->settings_.bus_label_offset_)
                    .SetFontSize(this->settings_.bus_label_font_size_)
                    .SetFontFamily("Verdana")
                    .SetData(r->number_);
                svg::Text routs_num_back_1 = svg::Text()
                    .SetFillColor(this->settings_.underlayer_color_)
                    .SetStrokeColor(this->settings_.underlayer_color_)
                    .SetStrokeWidth(this->settings_.underlayer_width_)
                    .SetFontWeight("bold")
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                    .SetPosition(scaler(r->da_way_[0]->geo_tag_))
                    .SetOffset(this->settings_.bus_label_offset_)
                    .SetFontSize(this->settings_.bus_label_font_size_)
                    .SetFontFamily("Verdana")
                    .SetData(r->number_);
                    routs_nums.push_back(routs_num_back_1);
                    routs_nums.push_back(routs_num_first);
                if (r->da_way_.size() > 1 && r->da_way_[0]->name_ != r->da_way_[r->da_way_.size() - 1]->name_) {
                    svg::Text routs_num_second = svg::Text()
                        .SetFillColor(this->settings_.color_palette_[ptr_clr])
                        .SetFontWeight("bold")
                        .SetPosition(scaler(r->da_way_[r->da_way_.size() - 1]->geo_tag_))
                        .SetOffset(this->settings_.bus_label_offset_)
                        .SetFontSize(this->settings_.bus_label_font_size_)
                        .SetFontFamily("Verdana")
                        .SetData(r->number_);
                    svg::Text routs_num_back_2 = svg::Text()
                        .SetFillColor(this->settings_.underlayer_color_)
                        .SetStrokeColor(this->settings_.underlayer_color_)
                        .SetStrokeWidth(this->settings_.underlayer_width_)
                        .SetFontWeight("bold")
                        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                        .SetPosition(scaler(r->da_way_[r->da_way_.size() - 1]->geo_tag_))
                        .SetOffset(this->settings_.bus_label_offset_)
                        .SetFontSize(this->settings_.bus_label_font_size_)
                        .SetFontFamily("Verdana")
                        .SetData(r->number_);
                    routs_nums.push_back(routs_num_back_2);
                    routs_nums.push_back(routs_num_second);
                }
            }
            
            for (const auto& r_t : routs_nums) {
                svg_document.Add(r_t);
            }
            ptr_clr++;
            if (ptr_clr == this->settings_.color_palette_.size()) {
                ptr_clr = 0;
            }
        }

        for (elements::Stop* s : stops_to_draw) {
                svg::Circle stop_round = svg::Circle()
                    .SetCenter(scaler(s->geo_tag_))
                    .SetRadius(this->settings_.stop_radius_)
                    .SetFillColor("white"s);
                svg_document.Add(stop_round);
        }

        for (elements::Stop* s : stops_to_draw) {
                svg::Text routs_back = svg::Text()
                    .SetData(s->name_)
                    .SetPosition(scaler(s->geo_tag_))
                    .SetOffset(this->settings_.stop_label_offset_)
                    .SetFontSize(this->settings_.stop_label_font_size_)
                    .SetFontFamily("Verdana")
                    .SetFontWeight({})
                    .SetFillColor(this->settings_.underlayer_color_)
                    .SetStrokeColor(this->settings_.underlayer_color_)
                    .SetStrokeWidth(this->settings_.underlayer_width_)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
                svg::Text routs_num = svg::Text()
                    .SetData(s->name_)
                    .SetPosition(scaler(s->geo_tag_))
                    .SetOffset(this->settings_.stop_label_offset_)
                    .SetFontSize(this->settings_.stop_label_font_size_)
                    .SetFontFamily("Verdana")
                    .SetFontWeight({})
                    .SetFillColor("black");
                svg_document.Add(routs_back);
                svg_document.Add(routs_num);
            }
        

        std::stringstream render_stream;
        svg_document.Render(render_stream);
        return render_stream.str();
	}

	void map_renderer::Set_settings(const renderSettings& settings) {
		this->settings_ = settings;
	}

	void map_renderer::Set_catalogue(const database::TransportCatalogue& catalogue) {
		this->catalogue_ = catalogue;
	}

	void map_renderer::Test_all_settings() {
		std::cout << this->settings_.width_ << std::endl;
		std::cout << this->settings_.height_ << std::endl;
		std::cout << this->settings_.padding_ << std::endl;
		std::cout << this->settings_.line_width_ << std::endl;
		std::cout << this->settings_.stop_radius_ << std::endl;
		std::cout << this->settings_.bus_label_font_size_ << std::endl;
		std::cout << this->settings_.underlayer_width_ << std::endl;
		std::cout << this->settings_.stop_label_font_size_ << std::endl;

		std::cout << std::setprecision(9) << this->settings_.bus_label_offset_.x << ':' << this->settings_.bus_label_offset_.y << std::endl;
		std::cout << std::setprecision(9) << this->settings_.stop_label_offset_.x << ':' << this->settings_.stop_label_offset_.y << std::endl;

		std::cout << this->settings_.underlayer_color_ << std::endl;
		for (const auto& C : this->settings_.color_palette_) {
			std::cout << C << std::endl;
		}
	}

} //namespace rnd
