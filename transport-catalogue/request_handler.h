#pragma once

#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"

#include <iostream>

namespace catalogue {
    namespace request_handler {
        class RequestHandler {
        public:
            RequestHandler(const catalogue::database::TransportCatalogue& database, const svg::MapRenderer& renderer);

            domain::RouteStats GetRouteStats(const std::string_view& route_name) const;

            domain::StopStats GetStopStats(const std::string_view& stop_name) const;

            void RenderMap(std::ostream& output) const;

        private:
            const catalogue::database::TransportCatalogue& database_;
            const svg::MapRenderer& renderer_;
        };
    } //namespace request_handler
} //namespace catalogue

