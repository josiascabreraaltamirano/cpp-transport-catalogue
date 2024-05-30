#pragma once

#include "domain.h"
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

#include <deque>
#include <unordered_map>
#include <utility>
#include <iostream>

namespace json {
    namespace input {
        using namespace catalogue;

        class JsonReader {
        public:
            JsonReader() = default;
            JsonReader& ParseJson(std::istream& input);
            const std::vector<Dict>& GetBaseRequests() const;
            const std::vector<Dict>& GetStatRequests() const;
            const Dict& GetRenderSettings() const;
            
        private:
            std::vector<Dict> ParseRequest(Array request);
            std::vector<Dict> base_requests_;
            std::vector<Dict> stat_requests_;
            Dict render_settings_;
        };

        struct BaseRequests {
            struct StopData {
                StopData(std::string name, 
                double lat, 
                double lon)
                : name(std::move(name))
                , latitude(lat)
                , longitude(lon)
                {
                }

                std::string name;
                double latitude;
                double longitude;
            };

            struct BusData {
                BusData(std::string name, 
                std::vector<std::string> stops,
                bool is_roundtrip) 
                : name(std::move(name))
                , stops(std::move(stops))
                , is_roundtrip(is_roundtrip)
                {
                }

                std::string name;
                std::vector<std::string> stops;
                bool is_roundtrip;
            };
            
            struct DistanceData {
                DistanceData(const std::string& from, 
                std::unordered_map<std::string, domain::Distance> distances) 
                : from(from)
                , road_distances(std::move(distances))
                {
                }

                std::string from;
                std::unordered_map<std::string, domain::Distance> road_distances;
            };
            
            std::deque<StopData> stops;
            std::deque<BusData> buses;
            std::deque<DistanceData> distances;
        };
        
        struct StatRequest {
            StatRequest(int id, 
                        std::string type,
                        std::string name) 
            : id(id)
            , type(std::move(type))
            , name(std::move(name))
            {
            }

            int id;
            std::string type;
            std::string name;
        };

        using StatRequests = std::vector<StatRequest>;
        
        class InputStandardizer {
        public:
            InputStandardizer() = default;
            void StandardizeBaseRequests(const std::vector<Dict>& base_requests);
            void StandardizeStatRequests(const std::vector<Dict>& stat_requests);
            void StandardizeRenderSettings(const Dict& render_settings);
            std::shared_ptr<BaseRequests> GetBaseRequests() const;
            std::shared_ptr<StatRequests> GetStatRequests() const;
            std::shared_ptr<svg::Settings> GetRenderSettings() const;

        private:
            BaseRequests base_requests_;
            StatRequests stat_requests_;
            svg::Settings render_settings_;
        };

        struct Requests {
            Requests(std::shared_ptr<BaseRequests> base, 
                     std::shared_ptr<StatRequests> stat,
                     std::shared_ptr<svg::Settings> settings) 
            : base_requests(base)
            , stat_requests(stat)
            , render_settings(settings)
            {
            }

            std::shared_ptr<BaseRequests> base_requests;
            std::shared_ptr<StatRequests> stat_requests;
            std::shared_ptr<svg::Settings> render_settings;
        };
        
        Requests ParseInput(std::istream& input);

        void ApplyBaseRequests(catalogue::database::TransportCatalogue& database,
                                                    std::shared_ptr<BaseRequests> base_requests);
    } //namespace reader
} // namespace json
