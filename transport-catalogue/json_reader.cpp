#include "json_reader.h"

#include <cassert>
#include <sstream>
#include <exception>

namespace json {
    namespace input {
        //JsonReader member functions definition
        JsonReader& JsonReader::ParseJson(std::istream& input) {
            using namespace std::literals;
            try {
                auto node_root = Load(input).GetRoot().AsMap();
                base_requests_ = std::move(ParseRequest(node_root.at("base_requests"s).AsArray()));
                stat_requests_ = std::move(ParseRequest(node_root.at("stat_requests"s).AsArray()));
                render_settings_ = std::move(node_root.at("render_settings"s).AsMap());
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
        
        std::vector<Dict> JsonReader::ParseRequest(Array request) {
            using namespace std::literals;

            std::vector<Dict> container;

            try {
                for (const auto& r : request) {
                    container.push_back(std::move(r.AsMap()));
                }
            } catch (const std::exception& e) {
                std::cerr << "error while parsing requests : "sv << e.what() << '\n';
            }

            return container;
        }

        //RequestStandardizer member functions definition
        void InputStandardizer::StandardizeBaseRequests(const std::vector<Dict>& base_requests) {
            using namespace std::literals;
            try {
                for (const auto& request : base_requests) {
                    auto name = request.at("name"s).AsString();
                    if (std::string_view type = request.at("type"s).AsString();
                                                            type == "Stop"sv) {
                        StandardizeStopData(request, std::move(name));
                        continue;
                    } else if (type == "Bus"sv) {
                        StandardizeBusData(request, std::move(name));
                        continue;
                    } else {
                        throw ParsingError("Unexpected type \""s + std::string(type) + "\" was found"s);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "error while parsing base request : "sv << e.what() << '\n';
            }     
        }
        
        void InputStandardizer::StandardizeStatRequests(const std::vector<Dict>& stat_requests) {
            using namespace std::literals;
            try {
                for (const auto& request : stat_requests) {
                    auto id = request.at("id"s).AsInt();
                    auto type = request.at("type").AsString();
                    if (auto name = request.find("name"s); name != request.end()) {
                        stat_requests_.emplace_back(id, std::move(type), std::move(name -> second.AsString()));
                        continue;
                    }
                    stat_requests_.emplace_back(id, std::move(type), ""s);
                }
            } catch (const std::exception& e) {
                std::cerr << "error while parsing stat requests : "sv << e.what() << '\n';
            }
        }

        void InputStandardizer::StandardizeRenderSettings(const Dict& render_settings) {
            using namespace std::literals;
            try {
                double width = render_settings.at("width"s).AsDouble();
                double height = render_settings.at("height"s).AsDouble();
                double padding = render_settings.at("padding"s).AsDouble();
                double line_width = render_settings.at("line_width"s).AsDouble();
                double stop_radius = render_settings.at("stop_radius"s).AsDouble();
                double underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
                int bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
                int stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();
                auto bus_label_offset = StandardizeOffsetData(render_settings.at("bus_label_offset"s).AsArray());
                auto stop_label_offset = StandardizeOffsetData(render_settings.at("stop_label_offset"s).AsArray());
                auto underlayer_color = StandardizeColorData(render_settings.at("underlayer_color"s));
                std::vector<svg::Color> color_palette;
                //Standardize the colors of the color palette
                for (const auto& color : render_settings.at("color_palette"s).AsArray()) {
                    color_palette.push_back(StandardizeColorData(color));
                }
                render_settings_ = svg::Settings{width, height, padding, line_width, stop_radius, bus_label_font_size, 
                bus_label_offset, stop_label_font_size, stop_label_offset, underlayer_color, underlayer_width, 
                color_palette};
            } catch (const std::exception& e) {
                std::cerr << "error while parsing render settings : "sv << e.what() << '\n';
            }
        }

        std::shared_ptr<BaseRequests> InputStandardizer::GetBaseRequests() const {
            return std::make_shared<BaseRequests>(base_requests_);
        }

        std::shared_ptr<StatRequests> InputStandardizer::GetStatRequests() const {
            return std::make_shared<StatRequests>(stat_requests_);
        }

        std::shared_ptr<svg::Settings> InputStandardizer::GetRenderSettings() const {
            return std::make_shared<svg::Settings>(render_settings_);
        }

        //private member functions
        void InputStandardizer::StandardizeStopData(const Dict& request, std::string stopname) {
            using namespace std::literals;

            auto latitude = request.at("latitude"s).AsDouble();
            auto longitude = request.at("longitude"s).AsDouble();
            //get the map of road distances in nodes
            auto road_nodes = request.at("road_distances"s).AsMap();
            //declare the var where road distances will be stored after parsing
            std::unordered_map<std::string, domain::Distance> road_distances;
            //reserve memory
            road_distances.reserve(road_nodes.size());
            for (auto& [stopname, distance] : road_nodes) {
                road_distances[std::move(stopname)] = distance.AsInt();
            }
            base_requests_.stops.emplace_back(std::move(stopname), latitude, longitude);
            base_requests_.distances.emplace_back(base_requests_.stops.back().name, std::move(road_distances));
        }

        void InputStandardizer::StandardizeBusData(const Dict& request, std::string busname) {
            using namespace std::literals;

            //false if the bus is using the same route to get to the initial stop
            bool is_roundtrip = request.at("is_roundtrip"s).AsBool();
            //get the vector of stops in nodes
            auto stop_nodes = request.at("stops"s).AsArray();
            //declare the var where stops will be stored after parsing
            std::vector<std::string> stops;
            //reserve memory
            stops.reserve(stop_nodes.size());
            for (const auto& stop : stop_nodes) {
                stops.push_back(stop.AsString());
            }
            base_requests_.buses.emplace_back(std::move(busname), std::move(stops), is_roundtrip);
        }

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

            if (auto size = offset.size(); size == 2) {
                return svg::Text::Offset{offset[0].AsDouble(), offset[1].AsDouble()};
            } else {
                throw ParsingError("Unexpected Offset format. There were found "s + std::to_string(size) + " values instead of 2"s);
            } 
        }
        //non-member functions
        Requests ParseInput(std::istream& input) {
            JsonReader reader;
            InputStandardizer parser;
            reader.ParseJson(input);
            parser.StandardizeBaseRequests(reader.GetBaseRequests());
            parser.StandardizeStatRequests(reader.GetStatRequests());
            parser.StandardizeRenderSettings(reader.GetRenderSettings());
            return Requests{parser.GetBaseRequests(), parser.GetStatRequests(), parser.GetRenderSettings()};
        }

        void ApplyBaseRequests(catalogue::database::TransportCatalogue& database,
                                                    std::shared_ptr<BaseRequests> input) {
            //check if the pointer has a value
            assert(input);
            //add the stops
            for (const auto& stop : input -> stops) {
                database.AddStop(stop.name, {stop.latitude, stop.longitude});
            }

            //add buses
            for (const auto& bus : input -> buses) {
                database.AddRoute(bus.name, bus.stops, bus.is_roundtrip);
            }
            
            //add distances
            for (const auto& distance : input -> distances) {
                for (const auto& [to, length] : distance.road_distances) {
                    database.SetDistance(distance.from, to, length);
                }
            }
        }

        
    } //namespace input

    namespace output {
        using namespace catalogue;
        void PrintStats(const request_handler::RequestHandler& handler, 
                                std::shared_ptr<json::input::StatRequests> stat_requests,
                                std::ostream& output) {
            using namespace std::literals;
            using namespace json;

            try {
                
                if (!stat_requests) {
                    throw std::runtime_error("Attemp to use an invalid pointer to access stat_requests"s);
                }
                
                Array root;
                //especial value to not found request
                std::pair<std::string, Node> not_found = {"error_message"s, Node{"not found"s}};

                for (const auto& request : *stat_requests) {
                    Dict request_response;
                    request_response["request_id"s] = Node{request.id};
                    if (request.type == "Bus"sv) {
                            auto route = handler.GetRouteStats(request.name);
                            if (route.is_route) {
                                request_response["curvature"s] = Node{static_cast<double>(route.curvature)};
                                request_response["route_length"s] = Node{static_cast<double>(route.length)};
                                request_response["unique_stop_count"s] = Node{route.unique_stops};
                                request_response["stop_count"s] = Node{route.total_stops};
                            } else {
                                request_response.insert(not_found);
                            }
                    } else if (request.type == "Stop"sv) {
                            auto stop = handler.GetStopStats(request.name);
                            if (stop.is_stop) {
                                Array routes_passing_by;
                                if (stop.routes) {
                                    for (const auto& route : *(stop.routes)) {
                                        routes_passing_by.push_back({route -> name});
                                    }
                                }
                                request_response["buses"s] = Node{routes_passing_by};

                            } else {
                                request_response.insert(not_found);
                            }
                    } else if (request.type == "Map"sv) {
                        std::ostringstream map_stream;
                        handler.RenderMap(map_stream);
                        request_response["map"] = Node{map_stream.str()};
                    } else {
                        throw std::invalid_argument("Unknown type \""s + request.type + "\"."s);
                    }
                    
                    root.push_back({request_response});
                }
                json::Print(json::Document{Node{root}}, output);
                
            } catch (const std::exception& e) {
                throw std::runtime_error("something went wrong while handling stat requests : "s + e.what());
            }
        }
    }//namespace output
} // namespace json
