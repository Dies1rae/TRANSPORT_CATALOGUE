#include "svg.h"

namespace svg {
    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();
        RenderObject(context);
        context.out << std::endl;
    }

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << this->center_.x << "\" cy=\""sv << this->center_.y << "\" "sv;
        out << "r=\""sv << this->radius_ << "\""sv;
        this->RenderAttrs(context.out);
        out << "/>"sv;
    }

    Polyline& Polyline::AddPoint(const Point& point) {
        this->hi_points_.emplace_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        bool firstPoint = true;
        for (const auto& p : this->hi_points_) {
            out << (firstPoint ? ""sv : " "sv) << p.x << ","sv << p.y;
            firstPoint = false;
        }
        out << "\""sv;
        this->RenderAttrs(context.out);
        out << "/>"sv;
    }

    Text& Text::SetPosition(const Point& pos) {
        this->main_pos_ = pos;
        return *this;
    }

    Text& Text::SetOffset(const Point& offset) {
        this->offset_pos_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        this->text_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(const std::string& font_family) {
        this->f_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(const std::string& font_weight) {
        this->f_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(const std::string& data) {
        this->main_text_ = data;
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text";
        this->RenderAttrs(context.out);
        out << " x=\""sv << this->main_pos_.x << "\" y=\""sv << this->main_pos_.y << "\" "sv;
        out << "dx=\""sv << this->offset_pos_.x << "\" dy=\""sv << this->offset_pos_.y << "\" "sv;
        out << "font-size=\""sv << this->text_size_ << "\"";
        if (this->f_family_.size() != 0) {
            out << " font-family=\""sv << this->f_family_ << "\""sv;
        }
        if (this->f_weight_.size() != 0 && this->f_weight_ != "normal"sv) {
            out << " font-weight=\""sv << this->f_weight_ << "\""sv;
        }
        
        out << ">"sv;
        for (char c : this->main_text_) {
            switch (c) {
            case '"': out << "&quot;"sv; break;
            case '\'': out << "&apos;"sv; break;
            case '<': out << "&lt;"sv; break;
            case '>': out << "&gt;"sv; break;
            case '&': out << "&amp;"sv; break;
            default:
                out << c;
            }
        }
        out << "</text>"sv;
    }

    void Document::AddPtr(std::shared_ptr<Object>&& obj) {
        this->obj_base_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
        for (size_t ptr = 0; ptr < this->obj_base_.size(); ++ptr) {
            out << "  ";
            this->obj_base_.at(ptr)->Render(out);
        }
        out << "</svg>"sv;
    }

    void Document::Render(std::stringstream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
        for (size_t ptr = 0; ptr < this->obj_base_.size(); ++ptr) {
            out << "  ";
            this->obj_base_.at(ptr)->Render(out);
        }
        out << "</svg>"sv;
    }
}  // namespace svg
