#include "svg.h"

namespace svg {

    using namespace std::literals;

    // ---------- ColorRendering ------------------
    
    ColorRendering::ColorRendering(std::ostream& out_stream) 
    : out(out_stream)
    {
    }

    void ColorRendering::operator()(std::monostate) const {
        out << "none"sv;
    }

    void ColorRendering::operator()(std::string value) const {
        out << value;
    }

    void ColorRendering::operator()(Rgb rgb) const {
        out << "rgb("sv << +rgb.red << ',' << +rgb.green << ',' << +rgb.blue << ')';
    }
    
    void ColorRendering::operator()(Rgba rgba) const {
        out << "rgba("sv << +rgba.red << ',' << +rgba.green << ',' << +rgba.blue << ','
        << +rgba.opacity << ')';
    }

    std::ostream& operator<<(std::ostream& output, const Color& color) {
        std::visit(ColorRendering{output}, color);
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const StrokeLineCap& value) {
        switch (value) {
            case StrokeLineCap::BUTT :
                output << "butt"sv;
                break;
            
            case StrokeLineCap::ROUND :
                output << "round"sv;
                break;
            
            case StrokeLineCap::SQUARE :
                output << "square"sv;
                break;
        }
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const StrokeLineJoin& value) {
        switch (value) {
            case StrokeLineJoin::ARCS :
                output << "arcs"sv;
                break;
            
            case StrokeLineJoin::BEVEL :
                output << "bevel"sv;
                break;
            
            case StrokeLineJoin::MITER :
                output << "miter"sv;
                break;

            case StrokeLineJoin::MITER_CLIP :
                output << "miter-clip"sv;
                break;
            
            case StrokeLineJoin::ROUND :
                output << "round"sv;
                break;
        }
        return output;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center)  {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius)  {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        //opens the tag
        out << "<circle"sv;
        //render the circle attributes
        out << " cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        //render its other attributes
        RenderAttrs(out);
        //closes the tag
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        //opens the tag
        out << "<polyline "sv;
        //render the points
        out << "points=\""sv;
        bool is_first = true; 
        for (const auto& point : points_) {
            if (!is_first) {
                out << " "sv;
            }
            out << point.x << ","sv << point.y;
            is_first = false; 
        }
        out.put('\"');
        //render its other attributes
        RenderAttrs(out);
        //closes the tag
        out << "/>"sv;
    }

    // ---------- Text------------------
    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Offset offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        //opening tag
        out << "<text"sv;
        RenderAttrs(out);
        out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
        out << "dx=\""sv << offset_.dx << "\" dy=\""sv << offset_.dy << "\" "sv;
        out << "font-size=\""sv << font_size_;
        if (!font_family_.empty()) {
            out << "\" "sv << "font-family=\""sv << font_family_;
        }
        if (!font_weight_.empty()) {
            out << "\" "sv << "font-weight=\""sv << font_weight_; 
        }
        out << "\">"sv;
        
        //text
        for (const auto c : data_) {
            switch (c) {
            case '\"' :
                out << "\""sv;
                break;

            case '<' :
                out << "<"sv;
                break;
            case '>' :
                out << ">"sv;
                break;
            case '\'' :
                out << "&apos;"sv;
                break;
            case '&' :
                out << "&"sv;
                break;
            default:
                out << c;
            }
        }
        //closing tag
        out << "</text>"sv;
    }

    // ----------Document------------------
    // Добавляет в svg-документ объект-наследник svg::Object
    void Document::AddPtr(ObjectPtr&& obj) {
        objects_.push_back(std::move(obj));
    }

    // Выводит в ostream svg-представление документа
    void Document::Render(std::ostream& out) const {
        //opening tag
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << '\n';
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << '\n';
        //objects
        RenderContext rc(out, 2, 2);
        for (const auto& obj : objects_) {
            if (obj){
                obj -> Render(rc);
            } 
        }
        //closign tag
        out << "</svg>"sv << '\n'; 
    }

}  // namespace svg