#include "json_reader.h"

#include <cassert>

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
                        base_requests_.stops.emplace_back(std::move(name), latitude, longitude);
                        base_requests_.distances.emplace_back(base_requests_.stops.back().name, std::move(road_distances));
                        continue;
                    } else if (type == "Bus"sv) {
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
                        base_requests_.buses.emplace_back(std::move(name), std::move(stops), is_roundtrip);
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
                
                auto parse_offset = [] (const Array& offset) {
                    if (auto size = offset.size(); size == 2) {
                        return svg::Text::Offset{offset[0].AsDouble(), offset[1].AsDouble()};
                    } else {
                        throw ParsingError("Unexpected Offset format. There were found "s + std::to_string(size) + " values instead of 2"s);
                    } 
                };

                auto parse_color = [] (const Node& color_type) {
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
                };

                auto bus_label_offset = parse_offset(render_settings.at("bus_label_offset"s).AsArray());
                auto stop_label_offset = parse_offset(render_settings.at("stop_label_offset"s).AsArray());
                auto underlayer_color = parse_color(render_settings.at("underlayer_color"s));
                std::vector<svg::Color> color_palette;
                //Standardize the colors of the color palette
                for (const auto& color : render_settings.at("color_palette"s).AsArray()) {
                    color_palette.push_back(parse_color(color));
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
    } //namespace reader
} // namespace json
