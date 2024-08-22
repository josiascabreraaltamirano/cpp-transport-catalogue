#include "request_handler.h"

namespace catalogue {
    namespace request_handler {
        //Class RequestHandler
        RequestHandler::RequestHandler(const database::TransportCatalogue& database, 
                                       const router::TransportRouter& router, 
                                       const svg::MapRenderer& renderer) 
        : database_(database)
        , router_(router)
        , renderer_(renderer)
        {
        }
        
        domain::RouteStats RequestHandler::GetRouteStats(std::string_view route_name) const {
            return database_.GetRouteStats(route_name);
        }

        domain::StopStats RequestHandler::GetStopStats(std::string_view stop_name) const {
            return database_.GetStopStats(stop_name);
        }

        std::optional<router::TransportRouter::RoutePlan> RequestHandler::GetRoutePlan(std::string_view from, std::string_view to) const {
            return router_.BuildRoute(from, to);
        }

        void RequestHandler::RenderMap(std::ostream& output) const {
            renderer_.RenderMap(database_.GetActiveStops(), database_.GetActiveRoutes(), output);
        }
    } //namespace request_handler
} //namespace catalogue
