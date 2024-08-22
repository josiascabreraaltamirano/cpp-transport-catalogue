#include "json_reader.h" 
#include "json_builder.h" 

#include <cassert> 
#include <sstream> 
#include <exception> 

namespace json { 

    namespace input { 
        using namespace std::literals;
        //JsonReader member functions definition 
        JsonReader& JsonReader::ParseJson(std::istream& input) { 
            try { 
                auto node_root = Load(input).GetRoot().AsDict(); 
                base_requests_ = std::move(ParseRequest(node_root.at("base_requests"s).AsArray())); 
                stat_requests_ = std::move(ParseRequest(node_root.at("stat_requests"s).AsArray())); 
                render_settings_ = std::move(node_root.at("render_settings"s).AsDict());
                routing_settings_ =  std::move(node_root.at("routing_settings"s).AsDict());
            } catch (const std::exception& e) { 
                std::cerr << "error while parsing json document : "sv << e.what() << '\n'; 
            }

            return *this; 
        } 

        const std::vector<Dict>& JsonReader::GetBaseRequests() const { 
            return base_requests_; 
        } 

        const std::vector<Dict>& JsonReader::GetStatRequests() const { 
            return stat_requests_; 
        } 

        const Dict& JsonReader::GetRenderSettings() const { 
            return render_settings_; 
        } 

        const Dict& JsonReader::GetRoutingSettings() const { 
            return routing_settings_; 
        } 

        std::vector<Dict> JsonReader::ParseRequest(Array request) {  
            std::vector<Dict> container; 
            try { 
                for (const auto& r : request) { 
                    container.push_back(std::move(r.AsDict())); 
                } 
            } catch (const std::exception& e) { 
                std::cerr << "error while parsing requests : "sv << e.what() << '\n'; 
            } 
            return container; 
        } 

        //RequestStandardizer static functions definition 
        BaseRequests InputStandardizer::StandardizeBaseRequests(const std::vector<Dict>& base_requests) {
            BaseRequests result; 

            try { 
                for (const Dict& request : base_requests) { 
                    auto name = request.at("name"s).AsString(); 
                    if (auto type = request.at("type"s).AsString(); type == "Stop"sv) { 
                        std::unordered_map<std::string, domain::Distance> road_distances; 

                        for (auto& [to, distance] : request.at("road_distances"s).AsDict()) { 
                            road_distances[std::move(to)] = distance.AsInt(); 
                        }
                        
                        result.Add(BaseRequests::StopData{}
                                                .SetName(name)
                                                .SetLatitude(request.at("latitude"s).AsDouble())
                                                .SetLongitude(request.at("longitude"s).AsDouble()));
                        
                        result.Add(BaseRequests::DistanceData{}
                                                .SetFrom(std::move(name))
                                                .SetRoadDistances(std::move(road_distances)));

                    } else if (type == "Bus"sv) { 
                        //get the vector of stops in nodes 
                        auto stop_nodes = request.at("stops"s).AsArray(); 
                        //declare the var where stops will be stored after parsing 
                        std::vector<std::string> stops; 
                        //reserve memory 
                        stops.reserve(stop_nodes.size()); 

                        for (const auto& stop : stop_nodes) { 
                            stops.push_back(stop.AsString()); 
                        } 

                        result.Add(BaseRequests::BusData{}
                                                .SetName(std::move(name))
                                                .SetStops(std::move(stops))
                                                .SetIsRoundtrip(request.at("is_roundtrip"s).AsBool())); 
                    } else { 
                        throw ParsingError("Unexpected type \""s + std::string(type) + "\" was found"s); 
                    } 
                } 
            } catch (const std::exception& e) { 
                std::cerr << "error while parsing base request : "sv << e.what() << '\n'; 
            }

            return result;       
        } 

        StatRequests InputStandardizer::StandardizeStatRequests(const std::vector<Dict>& stat_requests) { 
            StatRequests result;
            try { 
                for (const auto& request : stat_requests) { 
                    auto id = request.at("id"s).AsInt(); 
                    auto type = request.at("type").AsString(); 
                    
                    if (type == "Route"sv) {
                        result.Add(StatRequests::Router{}.SetId(id)
                                                         .SetType(std::move(type))
                                                         .SetFrom(request.at("from"s).AsString())
                                                         .SetTo(request.at("to"s).AsString()));
                        continue;                                                                 
                    }
                    auto name_iter = request.find("name"s);
                    result.Add(StatRequests::Transport{}.SetId(id)
                                                                .SetType(std::move(type))
                                                                .SetName(name_iter != request.end() ? 
                                                                name_iter -> second.AsString() : std::string{})); 
                } 
            } catch (const std::exception& e) { 
                std::cerr << "error while parsing stat requests : "sv << e.what() << '\n'; 
            }

            return result; 
        } 

        svg::Settings InputStandardizer::StandardizeRenderSettings(const Dict& render_settings) { 
            try { 
                std::vector<svg::Color> color_palette; 
                //Standardize the colors of the color palette 
                for (const auto& color : render_settings.at("color_palette"s).AsArray()) { 
                    color_palette.push_back(StandardizeColorData(color)); 
                } 

                return svg::Settings{}.SetWidth(render_settings.at("width"s).AsDouble())
                                                  .SetHeight(render_settings.at("height"s).AsDouble())
                                                  .SetPadding(render_settings.at("padding"s).AsDouble())
                                                  .SetLineWidth(render_settings.at("line_width"s).AsDouble())
                                                  .SetStopRadius(render_settings.at("stop_radius"s).AsDouble())
                                                  .SetUnderlayerWidth(render_settings.at("underlayer_width"s).AsDouble())
                                                  .SetBusLabelFontSize(render_settings.at("bus_label_font_size"s).AsInt())
                                                  .SetStopLabelFontSize(render_settings.at("stop_label_font_size"s).AsInt())
                                                  .SetBusLabelOffset(StandardizeOffsetData(render_settings.at("bus_label_offset"s).AsArray()))
                                                  .SetStopLabelOffset(StandardizeOffsetData(render_settings.at("stop_label_offset"s).AsArray()))
                                                  .SetUnderlayerColor(StandardizeColorData(render_settings.at("underlayer_color"s)))
                                                  .SetColorPalette(std::move(color_palette)); 
            } catch (const std::exception& e) { 
                std::cerr << "error while parsing render settings : "sv << e.what() << '\n'; 
            } 

            return {};
        } 

        RouterSettings InputStandardizer::StandardizeRoutingSettings(const Dict& routing_settings) {
            try { 
                return RouterSettings{}.SetBusVelocity(routing_settings.at("bus_velocity"s).AsDouble())
                                        .SetBusWaitTime(routing_settings.at("bus_wait_time"s).AsInt());
            } catch (const std::exception& e) { 
                std::cerr << "error while parsing render settings : "sv << e.what() << '\n'; 
            } 

            return {};
        }

        //private member functions 
        svg::Color InputStandardizer::StandardizeColorData(const Node& color_type) { 
            using namespace std::literals; 
            svg::Color color; 

            if (color_type.IsString()) { 
                color = color_type.AsString(); 
            } else if (color_type.IsArray()) { //should be rgb or rgba 
                auto rgb = color_type.AsArray(); 
                uint8_t r = rgb[0].AsInt(); 
                uint8_t g = rgb[1].AsInt(); 
                uint8_t b = rgb[2].AsInt(); 

                //if rgba 
                if (auto size = rgb.size(); size == 4) { 
                    double a = rgb[3].AsDouble(); 
                    color = svg::Rgba(r, g, b, a); 
                } else { // it may be only rgb 
                    color = svg::Rgb(r, g, b); 
                } 
            } else { 
                throw ParsingError("Unexpected Color format"s); 
            } 
            return color; 
        } 

        svg::Text::Offset InputStandardizer::StandardizeOffsetData(const Array& offset) { 
            using namespace std::literals; 

            static const double OFFSET_EXPECTED_ELEMENTS = 2;

            if (auto size = offset.size(); OFFSET_EXPECTED_ELEMENTS == size) { 
                return svg::Text::Offset{}.SetDx(offset[0].AsDouble())
                                          .SetDy(offset[1].AsDouble()); 
            } else { 
                throw ParsingError("Unexpected Offset format. There were found "s + std::to_string(size) + " values instead of "s + std::to_string(OFFSET_EXPECTED_ELEMENTS)); 
            }  
        } 

        //non-member functions 
        Requests ParseInput(std::istream& input) { 
            JsonReader reader; 
            reader.ParseJson(input); 

            return Requests{}
                   .SetBaseRequests(InputStandardizer::StandardizeBaseRequests(reader.GetBaseRequests()))
                   .SetStatRequests(InputStandardizer::StandardizeStatRequests(reader.GetStatRequests()))
                   .SetRenderSettings(InputStandardizer::StandardizeRenderSettings(reader.GetRenderSettings()))
                   .SetRouterSettings(InputStandardizer::StandardizeRoutingSettings(reader.GetRoutingSettings()));
        } 

        void ApplyBaseRequests(catalogue::database::TransportCatalogue& database, 
                                                    const BaseRequests& input) { 
            //add the stops 
            for (const auto& stop : input.stops) { 
                database.AddStop(stop.name, {stop.latitude, stop.longitude}); 
            } 
            //add buses 
            for (const auto& bus : input.buses) { 
                database.AddRoute(bus.name, bus.stops, bus.is_roundtrip); 
            } 
            //add distances 
            for (const auto& distance : input.distances) { 
                for (const auto& [to, length] : distance.road_distances) { 
                    database.SetDistance(distance.from, to, length); 
                } 
            } 
        } 
    } //namespace input 

    namespace output { 
        void PrintStats(const request_handler::RequestHandler& handler,  
                        const StatRequests& stat_requests, 
                        std::ostream& output) { 
            using namespace std::literals; 
            using namespace json; 

            try {                 
                auto root = Builder{}; 
                auto root_as_array = root.StartArray(); 
                for (const auto& request : stat_requests.requests) { 
                    assert(request);

                    auto request_response = root_as_array.StartDict(); 
                    request_response.Key("request_id"s).Value(request -> id);
                    if (request -> type == "Route"sv) {
                        auto router_request = dynamic_cast<StatRequests::Router*>(request.get());

                        assert(router_request);

                        if (auto route_plan = handler.GetRoutePlan(router_request -> from,
                                                                   router_request -> to)) {
                            request_response.Key("total_time").Value(route_plan -> total_time);

                            auto items = request_response.Key("items"s).StartArray();
                            for (const auto& item : route_plan -> items) {
                                bool is_bus = item.span_count != 0;
                                auto element = items.StartDict();
                                element.Key("time").Value(item.weight);
                                if (is_bus) {
                                    element.Key("type"s).Value("Bus"s)
                                           .Key("bus"s).Value(std::string(item.name))
                                           .Key("span_count"s).Value(item.span_count);
                                } else {
                                    element.Key("type"s).Value("Wait"s)
                                           .Key("stop_name"s).Value(item.name);
                                }
                                element.EndDict();
                            }

                            items.EndArray();

                        } else {
                            request_response.Key("error_message"s).Value("not found"s);
                        }
                    } else {
                        auto transport_request = static_cast<StatRequests::Transport*>(request.get());
                        assert(transport_request);
                        
                        if (request -> type == "Bus"sv) { 
                            auto route = handler.GetRouteStats(transport_request -> name); 
                            if (route.is_route) { 
                                request_response 
                                .Key("curvature"s).Value(static_cast<double>(route.curvature)) 
                                .Key("route_length"s).Value(static_cast<double>(route.length)) 
                                .Key("unique_stop_count"s).Value(route.unique_stops) 
                                .Key("stop_count"s).Value(route.total_stops); 
                            } else { 
                                request_response.Key("error_message"s).Value("not found"s); 
                            } 

                        } else if (request -> type == "Stop"sv) { 
                                auto stop = handler.GetStopStats(transport_request -> name); 
                                if (stop.is_stop) { 
                                    auto routes_passing_by = request_response.Key("buses"s).StartArray(); 
                                    if (stop.routes) { 
                                        for (const auto& route : *(stop.routes)) { 
                                            routes_passing_by.Value(route -> name); 
                                        } 
                                    } 
                                    routes_passing_by.EndArray(); 
                                } else { 
                                    request_response.Key("error_message"s).Value("not found"s); 
                                } 

                        } else if (request -> type == "Map"sv) { 
                            std::ostringstream map_stream; 
                            handler.RenderMap(map_stream); 
                            request_response.Key("map"s).Value(map_stream.str()); 
                        } else { 
                            throw std::invalid_argument("Unknown type \""s + transport_request -> type + "\"."s); 
                        }
                    } 

                    request_response.EndDict(); 
                } 

                root_as_array.EndArray(); 
                json::Print(json::Document{root.Build()}, output);                 

            } catch (const std::exception& e) { 
                throw std::runtime_error("something went wrong while handling stat requests: "s + e.what()); 
            } 
        } 
    }//namespace output 

} // namespace json 

 