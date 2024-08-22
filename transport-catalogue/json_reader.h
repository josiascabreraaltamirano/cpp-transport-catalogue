#pragma once

#include "domain.h"
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "request_handler.h"

#include <deque>
#include <unordered_map>
#include <utility>
#include <iostream>

namespace json {
    using namespace catalogue;
    using namespace domain;

    namespace input {
        class JsonReader {
        public:
            JsonReader() = default;
            JsonReader& ParseJson(std::istream& input);
            const std::vector<Dict>& GetBaseRequests() const;
            const std::vector<Dict>& GetStatRequests() const;
            const Dict& GetRenderSettings() const;
            const Dict& GetRoutingSettings() const;
            
        private:
            std::vector<Dict> ParseRequest(Array request);

            std::vector<Dict> base_requests_;
            std::vector<Dict> stat_requests_;
            Dict render_settings_;
            Dict routing_settings_;
        };
 
        class InputStandardizer {
        public:
            static BaseRequests StandardizeBaseRequests(const std::vector<Dict>& base_requests);
            static StatRequests StandardizeStatRequests(const std::vector<Dict>& stat_requests);
            static svg::Settings StandardizeRenderSettings(const Dict& render_settings);
            static RouterSettings StandardizeRoutingSettings(const Dict& routing_settings);

        private:
            static svg::Text::Offset StandardizeOffsetData(const Array& offset);
            static svg::Color StandardizeColorData(const Node& color_type);
        };

        struct Requests {
            BaseRequests base_requests;
            StatRequests stat_requests;
            svg::Settings render_settings;
            RouterSettings router_settings;

            Requests& SetBaseRequests(BaseRequests&& requests) {
                base_requests = std::move(requests);
                return *this;
            }

            Requests& SetStatRequests(StatRequests&& requests) {
                stat_requests = std::move(requests);
                return *this;
            }

            Requests& SetRenderSettings(svg::Settings&& settings) {
                render_settings = std::move(settings);
                return *this;
            }

            Requests& SetRouterSettings(RouterSettings&& settings) {
                router_settings = std::move(settings);
                return *this;
            }
        };
        
        Requests ParseInput(std::istream& input);

        void ApplyBaseRequests(catalogue::database::TransportCatalogue& database,
                                                    const BaseRequests& base_requests);

        
    } //namespace input

    namespace output {
        using namespace catalogue;
        void PrintStats(const request_handler::RequestHandler& handler, 
                        const StatRequests& stat_requests,
                        std::ostream& output);
    } //namespace output
} // namespace json
