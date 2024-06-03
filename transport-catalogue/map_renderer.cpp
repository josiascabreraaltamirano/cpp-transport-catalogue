#include "map_renderer.h"

#include <algorithm>

namespace svg {
    bool IsZero(double value) {
            return std::abs(value) < EPSILON;
    }

    //class SphereProjector
    svg::Point SphereProjector::operator()(catalogue::geo::Coordinates coords) const {
            return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
    }

    //class RouteLine
    MapRenderer::RouteLine::RouteLine(const catalogue::domain::Route& route, 
                         const std::unordered_map<std::string_view, Point>& points, 
                         Color stroke_color, 
                         double stroke_width)
    : stroke_color_(stroke_color)
    , stroke_width_(stroke_width) 
    , is_roundtrip(route.is_roundtrip) {
        const auto& stops = route.stops;
        stops_.reserve(stops.size());
        for (const auto& stop : stops) {
            assert(stop);
            stops_.push_back(points.at(stop -> name));
        }
    }

    void MapRenderer::RouteLine::Draw(ObjectContainer& container) const {
        Polyline routeline;
        routeline.SetFillColor({})
        .SetStrokeColor(stroke_color_)
        .SetStrokeWidth(stroke_width_)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND);
        for (const auto& stop : stops_) {
            routeline.AddPoint(stop);
        }

        if (!is_roundtrip) {
            for (auto it = stops_.rbegin() + 1; it != stops_.rend(); it++) {
                routeline.AddPoint(*it);
            }
        }

        container.Add(std::move(routeline));
    }

    //class RouteName
    MapRenderer::RouteName::RouteName(const catalogue::domain::Route& route, 
                                      const std::unordered_map<std::string_view, Point>& points,
                                      Color color,
                                      const Settings& settings)
    : name_(route.name)
    , color_(std::move(color))
    , is_roundtrip_(route.is_roundtrip)
    , settings_(settings)
    {
        const auto& stops = route.stops;
        auto origin = stops.front();
        auto end = stops.back();
        assert(origin && end);
        origin_ = points.at(origin -> name);
        end_ = points.at(end -> name);
    }
    
    void MapRenderer::RouteName::Draw(ObjectContainer& container) const {
        using namespace std::literals;

        Text routename;
        routename.SetPosition(origin_)
                 .SetOffset(settings_.bus_label_offset)
                 .SetFontSize(settings_.bus_label_font_size)
                 .SetFontFamily("Verdana"s)
                 .SetFontWeight("bold"s)
                 .SetData(name_);
        
        Text underlayer = routename;
        underlayer.SetFillColor(settings_.underlayer_color)
                  .SetStrokeColor(settings_.underlayer_color)
                  .SetStrokeWidth(settings_.underlayer_width)
                  .SetStrokeLineCap(StrokeLineCap::ROUND)
                  .SetStrokeLineJoin(StrokeLineJoin::ROUND);
        
        routename.SetFillColor(color_);

        if (!is_roundtrip_ && origin_ != end_) {
            Text last_routename = routename;
            Text last_underlayer = underlayer;
            last_routename.SetPosition(end_);
            last_underlayer.SetPosition(end_);

            container.Add(std::move(underlayer));
            container.Add(std::move(routename));
            container.Add(std::move(last_underlayer));
            container.Add(std::move(last_routename));
            return;
        }

        container.Add(std::move(underlayer));
        container.Add(std::move(routename));
    }
    
    //class StopDot
    MapRenderer::StopDot::StopDot(Point center, double radius) 
    : center_(std::move(center))
    , radius_(radius)
    {
    }

    void MapRenderer::StopDot::Draw(ObjectContainer& container) const {
        using namespace std::literals;
        container.Add(Circle().SetCenter(center_).SetRadius(radius_).SetFillColor("white"s));
    }

    //class StopNames
    MapRenderer::StopName::StopName(const std::string& name,
                     Point position,
                     const Settings& settings) 
    : name_(name)
    , position_(std::move(position))
    , settings_(settings)
    {
    }

    void MapRenderer::StopName::Draw(ObjectContainer& container) const {
        using namespace std::literals;

        Text stopname;
        stopname.SetPosition(position_)
                 .SetOffset(settings_.stop_label_offset)
                 .SetFontSize(settings_.stop_label_font_size)
                 .SetFontFamily("Verdana"s)
                 .SetData(name_);
        
        Text underlayer = stopname;
        underlayer.SetFillColor(settings_.underlayer_color)
                  .SetStrokeColor(settings_.underlayer_color)
                  .SetStrokeWidth(settings_.underlayer_width)
                  .SetStrokeLineCap(StrokeLineCap::ROUND)
                  .SetStrokeLineJoin(StrokeLineJoin::ROUND);

        stopname.SetFillColor("black"s);

        container.Add(std::move(underlayer));
        container.Add(std::move(stopname));
    }

    //class MapRenderer
    MapRenderer::MapRenderer(const Settings& settings) 
    : settings_(settings) 
    {
    }

    void MapRenderer::RenderMap(std::vector<catalogue::domain::StopPtr> active_stops, 
                       std::vector<catalogue::domain::RoutePtr>active_routes, 
                       std::ostream& output) const {
        using namespace std::literals;

        //create a container of coordinates for creating SphereProjector
        std::vector<catalogue::geo::Coordinates> coordinates;
        for (const auto& stop : active_stops) {
            if (!stop) {
                throw std::runtime_error("An invalid point was tried to be used to access an Stop object"s);
            }
            coordinates.push_back(stop -> coordinates);
        }
        
        svg::SphereProjector conversor(coordinates.begin(), coordinates.end(),
                                        settings_.width, settings_.height,
                                        settings_.padding);

        //sort routes
        std::sort(active_routes.begin(), active_routes.end(), 
        [] (const catalogue::domain::RoutePtr& lhs, const catalogue::domain::RoutePtr& rhs) {
            assert(lhs && rhs);
            return lhs -> name < rhs -> name;
        });

        //sort stops
        std::sort(active_stops.begin(), active_stops.end(),
        [] (const catalogue::domain::StopPtr& lhs, const catalogue::domain::StopPtr& rhs) {
            assert(lhs && rhs);
            return lhs -> name < rhs -> name;
        });

        svg::Document document;
        RenderComponents(active_stops, active_routes, document, conversor);
        document.Render(output);
    }

    //private member functions
    void MapRenderer::RenderComponents(const std::vector<catalogue::domain::StopPtr>& stops, 
                                       const std::vector<catalogue::domain::RoutePtr>& routes,
                                       ObjectContainer& target, 
                                       const SphereProjector& conversor) const {

        using namespace std::literals;

        std::unordered_map<std::string_view, Point> stopname_to_point;
        for (const auto& stop : stops) {
            if (stop) {
                stopname_to_point[stop -> name] = conversor(stop -> coordinates);
            } 
        }
        
        //Temporary container
        std::vector<RouteLine> routelines;
        std::vector<RouteName> routenames;
        std::vector<StopDot> stopdots;
        std::vector<StopName> stopnames;

        //save a color iterator for passing it to the next route
        auto current_color = settings_.color_palette.begin();  
        for (const auto& route : routes) {
            //check wheter the pointer is valid
            if (!route) {
                std::cerr << "An invalid RoutePtr was tried to be used when rendering the Map"sv << '\n';
                return;
            }
            //if the container of colors is smaller than the amount of routes
            //restart the cycle and set the first color as current
            if (current_color == settings_.color_palette.end()) {
                current_color = settings_.color_palette.begin();
            }
            //create a RouteLine object
            routelines.emplace_back(*route, 
                                    stopname_to_point, 
                                    *current_color, 
                                    settings_.line_width);
            //create a RouteName object
            routenames.emplace_back(*route, 
                                    stopname_to_point,
                                    *current_color,
                                    settings_);
            //advance the iterator
            ++current_color;
        }

        for (const auto& stop : stops) {
            if (!stop) {
                std::cerr << "An invalid StopPtr was tried to be used when rendering the Map"sv << '\n';
                return;
            }
            stopdots.emplace_back(stopname_to_point.at(stop -> name), settings_.stop_radius);
            stopnames.emplace_back(stop -> name, stopname_to_point.at(stop -> name), settings_);
        }

        RenderMapElements(routelines, target);
        RenderMapElements(routenames, target);
        RenderMapElements(stopdots, target);
        RenderMapElements(stopnames, target);
    }

    

    

} //namespace svg
    
