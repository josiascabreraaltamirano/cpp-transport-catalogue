#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>

namespace svg {
    
    inline const std::string NoneColor{"none"};

    struct Rgb {
        Rgb() = default;
        
        Rgb(uint8_t red, uint8_t green, uint8_t blue) 
        : red(red)
        , green(green)
        , blue(blue)
        {
        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba {
        Rgba() = default;

        Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
        : red(red)
        , green(green)
        , blue(blue)
        , opacity(opacity)
        {
        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    struct ColorRendering {
        //constructor
        ColorRendering(std::ostream& out_stream);

        //data members
        std::ostream& out;

        //methods
        void operator()(std::monostate) const;

        void operator()(std::string value) const;
        
        void operator()(Rgb rgb) const;
        
        void operator()(Rgba rgba) const;
    };

    std::ostream& operator<<(std::ostream& output, const Color& color);
    
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

    
    std::ostream& operator<<(std::ostream& output, const StrokeLineCap& value);

    std::ostream& operator<<(std::ostream& output, const StrokeLineJoin& value);

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_line_cap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_line_join_ = line_join;
            return AsOwner();
        }

    protected:
        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                RenderAttr(out, "fill"sv, *fill_color_);
            }
            if (stroke_color_) {
                RenderAttr(out, "stroke"sv, *stroke_color_);
            }
            if (stroke_width_) {
                RenderAttr(out, "stroke-width"sv, *stroke_width_);
            }
            if (stroke_line_cap_) {
                RenderAttr(out, "stroke-linecap"sv, *stroke_line_cap_);
            }
            if (stroke_line_join_) {
                RenderAttr(out, "stroke-linejoin"sv, *stroke_line_join_);
            }
        }

        ~PathProps() = default;

    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        template<typename Value>
        void RenderAttr(std::ostream& out, std::string_view attr_name, const Value& value) const {
            using namespace std::literals;
            out.put(' ');
            out << attr_name << "=\""sv << value << "\""sv;
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
        
    };

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;

        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };

    inline bool operator!=(const Point& lhs, const Point& rhs) {
        return !(lhs == rhs);
    }


    /*
    * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
    * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
    */
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
            return {out, indent_step, indent + indent_step};
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

    /*
    * Абстрактный базовый класс Object служит для унифицированного хранения
    * конкретных тегов SVG-документа
    * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
    */

    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    /*
    * Класс Circle моделирует элемент <circle> для отображения круга
    * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
    */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    /*
    * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
    * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
    */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };

    /*
    * Класс Text моделирует элемент <text> для отображения текста
    * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
    */
    class Text final : public Object, public PathProps<Text> {
    public:
        struct Offset {
            double dx, dy;

            Offset& SetDx(double value) {
                dx = value;
                return *this;
            }

            Offset& SetDy(double value) {
                dy = value;
                return *this;
            }
        };
        
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Offset offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point position_;
        Offset offset_;
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;

        // Прочие данные и методы, необходимые для реализации элемента <text>
    };

    /*------------Class Intarface-------------*/
    class ObjectContainer {
    public:

    using ObjectPtr = std::unique_ptr<Object>;

        template <typename T>
        void Add(T obj) {
            objects_.emplace_back(std::make_unique<T>(std::move(obj)));
        }

        virtual void AddPtr(ObjectPtr&& obj) = 0;
        virtual ~ObjectContainer() = default;
    protected:
        std::vector<ObjectPtr> objects_;
    };

    class Document final : public ObjectContainer {
    public:
        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(ObjectPtr&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

    };

    /*------------Class Intarface-------------*/
    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;
        virtual ~Drawable() = default;
    };

}  // namespace svg