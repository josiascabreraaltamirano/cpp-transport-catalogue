#pragma once

#include "svg.h"
#include "geo.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <cassert>
#include <deque>
#include <unordered_map>

namespace svg {

    inline const double EPSILON = 1e-6;
    
    bool IsZero(double value);

    class SphereProjector {
    public:
        //SphereProjector() = default;
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding)
            : padding_(padding) //
        {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end) {
                return;
            }

            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            } else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            } else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(catalogue::geo::Coordinates coords) const;

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    struct Settings {
        Settings() = default;
        Settings(double width, double height,
                 double padding, double line_width,
                 double stop_radius, int bus_label_font_size,
                 Text::Offset bus_label_offset, int stop_label_font_size,
                 Text::Offset stop_label_offset, Color underlayer_color, 
                 double underlayer_width, std::vector<Color> color_palette)
        : width(width)
        , height(height)
        , padding(padding)
        , line_width(line_width)
        , stop_radius(stop_radius)
        , bus_label_font_size(bus_label_font_size)
        , bus_label_offset(bus_label_offset)
        , stop_label_font_size(stop_label_font_size)
        , stop_label_offset(stop_label_offset)
        , underlayer_color(underlayer_color)
        , underlayer_width(underlayer_width)
        , color_palette(std::move(color_palette))
        {
        }
        
    
        double width, height, padding, line_width, stop_radius;
        int bus_label_font_size;
        Text::Offset bus_label_offset; 
        int stop_label_font_size;
        Text::Offset stop_label_offset;
        Color underlayer_color;
        double underlayer_width;
        std::vector<Color> color_palette;
    };

    class MapRenderer final {
    public:
        class RouteLine : public Drawable {
        public:
            RouteLine(const catalogue::domain::Route& route, 
                    const std::unordered_map<std::string_view, Point>& points, 
                    Color stroke_color, 
                    double stroke_width);

            void Draw(ObjectContainer& container) const override;

        private:
            Color stroke_color_;
            double stroke_width_;
            bool is_roundtrip;
            std::vector<Point> stops_;
        };

        class RouteName : public Drawable {
        public:
            RouteName(const catalogue::domain::Route& route, 
                      const std::unordered_map<std::string_view, Point>& points,
                      Color color,
                      const Settings& settings);

            void Draw(ObjectContainer& container) const override;

        private:
            const std::string& name_;
            Color color_;
            bool is_roundtrip_;
            const Settings& settings_;
            Point origin_;
            Point end_; 
        };

        class StopDot : public Drawable {
        public:
            StopDot(Point center, double radius);

            void Draw(ObjectContainer& container) const override;

        private:
            Point center_;
            double radius_;
        };

        class StopName : public Drawable {
        public:
            StopName(const std::string& name,
                     Point position,
                     const Settings& settings);

            void Draw(ObjectContainer& container) const override;
        private:
            const std::string& name_;
            Point position_;
            const Settings& settings_;
        };

        MapRenderer(const Settings& settings);
        
        void RenderMap(std::vector<catalogue::domain::StopPtr> stops, std::vector<catalogue::domain::RoutePtr> routes,
                              ObjectContainer& target, 
                              const SphereProjector& conversor) const;

        const Settings& GetSettings() const;

    private:
        template<typename Container>
        void RenderMapElements(const Container& elements, ObjectContainer& target) const {
            for (const auto& element : elements) {
                element.Draw(target);
            }
        }

        const Settings& settings_;
    };

} //namespace svg