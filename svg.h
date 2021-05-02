#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <variant>
#include <cmath>
#include <sstream>

namespace svg {
    using namespace std::literals;

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    struct Rgb {
        Rgb() = default;
        Rgb(uint8_t red, uint8_t green, uint8_t blue): red(red), green(green), blue(blue) {}
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    inline std::ostream& operator<<(std::ostream& out, Rgb color) {
        using namespace std::literals;
        out << "rgb("sv << +color.red << ","sv << +color.green << ","sv << +color.blue << ")"sv;
        return out;
    }

    struct Rgba {
        Rgba() = default;
        Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity): red(red), green(green), blue(blue), opacity(opacity) {}
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    inline std::ostream& operator<<(std::ostream& out, Rgba color) {
        out << "rgba(" << +color.red << "," << +color.green << "," << +color.blue << "," << +color.opacity << ")";
        return out;
    }

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
    inline const Color NoneColor{ "none" };

    struct OstreamColorPrinter {
        std::ostream& out;
        void operator()(std::monostate) const {
            out << "none";
        }
        void operator()(std::string color) const {
            out << color;
        }
        void operator()(Rgb color) const {
            out << color;
        }
        void operator()(Rgba color) const {
            out << color;
        }
    };

    inline std::ostream& operator<<(std::ostream& out, Color color) {
        if (std::holds_alternative<std::monostate>(color)) {
            out << "none";
        } else if (std::holds_alternative<std::string>(color)) {
            out << std::get<std::string>(color);
        } else if (std::holds_alternative<Rgb>(color)) {
            out << std::get<Rgb>(color);
        } else if (std::holds_alternative<Rgba>(color)) {
            out << std::get<Rgba>(color);
        }

        return out;
    }

    namespace detail {
        inline uint8_t Lerp(uint8_t from, uint8_t to, double t) {
            return static_cast<uint8_t>(std::round((to - from) * t + from));
        }

    } // detail

    inline svg::Rgb Lerp(svg::Rgb from, svg::Rgb to, double t) {
        using namespace detail;
        return { Lerp(from.red, to.red, t), Lerp(from.green, to.green, t), Lerp(from.blue, to.blue, t) };
    }

    inline std::ostream& operator<< (std::ostream& out, const StrokeLineCap cap) {
        using namespace std::literals;
        switch (cap) {
            case StrokeLineCap::BUTT: out << "butt"sv; break;
            case StrokeLineCap::ROUND: out << "round"sv; break;
            case StrokeLineCap::SQUARE: out << "square"sv; break;
            default: break;
        }
        return out;
    }

    inline std::ostream& operator<< (std::ostream& out, const StrokeLineJoin join) {
        using namespace std::literals;
        switch (join) {
            case StrokeLineJoin::ARCS: out << "arcs"sv; break;
            case StrokeLineJoin::BEVEL: out << "bevel"sv; break;
            case StrokeLineJoin::MITER: out << "miter"sv; break;
            case StrokeLineJoin::MITER_CLIP: out << "miter-clip"sv; break;
            case StrokeLineJoin::ROUND: out << "round"sv; break;
            default: break;
        }
        return out;
    }

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            this->fill_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeColor(Color color) {
            this->stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            this->stroke_width_ = width;
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            this->line_cap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            this->stroke_line_join_ = line_join;
            return AsOwner();
        }
    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (line_cap_) {
                out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
            }
            if (stroke_line_join_) {
                out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
            }
        }
    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
    };

    struct Point {
    public:
        Point() = default;
        Point(double x, double y) : x(x), y(y) {}
        double x = 0;
        double y = 0;
    };

    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    class Object {
    public:
        virtual void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class ObjectContainer {
    public:
        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_shared<Obj>(std::move(obj)));
        }

        virtual void AddPtr(std::shared_ptr<Object>&& obj) = 0;

        virtual ~ObjectContainer() = default;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;

        virtual ~Drawable() = default;
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        Polyline& AddPoint(const Point& point);

    private:
        std::vector<Point> hi_points_;
        void RenderObject(const RenderContext& context) const override;
    };

    class Text final : public Object, public PathProps<Text> {
    public:
        Text& SetPosition(const Point& pos);

        Text& SetOffset(const Point& offset);

        Text& SetFontSize(uint32_t size);

        Text& SetFontFamily(const std::string& font_family);

        Text& SetFontWeight(const std::string& font_weight);

        Text& SetData(const std::string& data);

    private:
        uint32_t text_size_ = 1;
        Point main_pos_ = { 0,0 };
        Point offset_pos_ = { 0,0 };
        std::string f_family_;
        std::string f_weight_;
        std::string main_text_;
        void RenderObject(const RenderContext& context) const override;
    };


    class Document : public ObjectContainer {
    public:

        void AddPtr(std::shared_ptr<Object>&& obj);

        void Render(std::ostream& out) const;
        void Render(std::stringstream& out) const;
    private:
        std::vector<std::shared_ptr<Object>> obj_base_;
    };
}