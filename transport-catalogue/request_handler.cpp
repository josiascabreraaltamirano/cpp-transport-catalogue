#include "request_handler.h"

#include <algorithm>
#include <sstream>

namespace catalogue {
    namespace output {
        //Class RequestHandler
        RequestHandler::RequestHandler(const catalogue::database::TransportCatalogue& database,
                                       const svg::MapRenderer& renderer) 
        : database_(database)
        , renderer_(renderer)
        {
        }
        
        domain::RouteStats RequestHandler::GetRouteStats(const std::string_view& route_name) const {
            return database_.GetRouteStats(route_name);
        }

        domain::StopStats RequestHandler::GetStopStats(const std::string_view& stop_name) const {
            return database_.GetStopStats(stop_name);
        }

        void RequestHandler::RenderMap(std::ostream& output) const {
            using namespace std::literals;
            auto stops = database_.GetActiveStops();

            //create a container of coordinates for creating SphereProjector
            std::vector<catalogue::geo::Coordinates> coordinates;
            for (const auto& stop : stops) {
                if (!stop) {
                    throw std::runtime_error("An invalid point was tried to be used to access an Stop object"s);
                }
                coordinates.push_back(stop -> coordinates);
            }
            
            svg::SphereProjector conversor(coordinates.begin(), coordinates.end(),
                                           renderer_.GetSettings().width, renderer_.GetSettings().height,
                                           renderer_.GetSettings().padding);
            
            svg::Document document;
            renderer_.RenderMap(std::move(stops), database_.GetActiveRoutes(), document, conversor);
            document.Render(output);
        }

        void PrintStats(const RequestHandler& handler, 
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
                                        routes_passing_by.emplace_back(route -> name);
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
                    
                    root.emplace_back(request_response);
                }
                json::Print(json::Document{Node{root}}, output);
                
            } catch (const std::exception& e) {
                throw std::runtime_error("something went wrong while handling stat requests : "s + e.what());
            }
        }

        
    } //namespace output
} //namespace catalogue
