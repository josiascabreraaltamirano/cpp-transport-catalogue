#pragma once

#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_reader.h"
#include "json.h"

#include <iostream>
#include <optional>
#include <exception>
#include <cassert>

namespace catalogue {
    namespace output {
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


        void PrintStats(const RequestHandler& handler, 
                                std::shared_ptr<json::input::StatRequests> stat_requests,
                                std::ostream& output);


 

    } //namespace output
} //namespace catalogue

