#pragma once

#include "domain.h"
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <iostream>

namespace catalogue {
    namespace request_handler {
        class RequestHandler {
        public:
            RequestHandler(const database::TransportCatalogue& database, 
                           const router::TransportRouter& router, 
                           const svg::MapRenderer& renderer);

            domain::RouteStats GetRouteStats(std::string_view route_name) const;
            domain::StopStats GetStopStats(std::string_view stop_name) const;
            std::optional<router::TransportRouter::RoutePlan> GetRoutePlan(std::string_view from, std::string_view to) const;
            void RenderMap(std::ostream& output) const;

        private:
            const database::TransportCatalogue& database_;
            const router::TransportRouter& router_;
            const svg::MapRenderer& renderer_;
             
        };
    } //namespace request_handler
} //namespace catalogue

