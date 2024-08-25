#pragma once

#include "graph.h"
#include "router.h"
#include "domain.h"
#include "transport_catalogue.h"

#include <vector>
#include <unordered_map>
#include <utility>
#include <cassert>
#include <stdexcept>

namespace catalogue {
    namespace router {

        using Database = database::TransportCatalogue;

        class TransportRouter {
        private:
            using Time = double;
            using Stop = domain::StopPtr;
            using Graph = graph::DoubleVertexGraph<Time, Stop>;
            using Router = graph::Router<Time>;

        public:
            struct RoutePlan {
                Time total_time;
                std::vector<Graph::EdgeSegmentInfo> items;
            };

            TransportRouter(const Database& source, const domain::RouterSettings& settings);
            std::optional<RoutePlan> BuildRoute(std::string_view from, std::string_view to) const;

        private:
            RoutePlan ProcessRouteInfo(const Router::RouteInfo& route_info) const;

            class TransportGraphFactory {
            public:
                TransportGraphFactory(const Database& source, const domain::RouterSettings& settings)
                : database_(source)
                , settings_(settings)
                {  
                }

                /*
                Я решил оставить эту строку без изменений, 
                поскольку считаю, что этот класс должен работать 
                как конструктор Graph и изолировать эту функциональность 
                от остального кода. Таким образом, если функция возвращает 
                Graph, я могу присвоить его непосредственно полю класса 
                Transport Router. Пожалуйста, позвольте мне оставить это как есть. 
                В любом случае, если вы сочтете, что есть лучший подход, 
                дайте мне знать, и я изменю его.
                */
                Graph MakeTransportGraph() {
                    Graph graph(database_.GetActiveStops());
                    MakeBusesEdges(graph);
                    return graph;
                }

            private:
                template <typename Iter>
                struct BusStopsData {
                    std::string_view busname;
                    Iter first;
                    Iter last;
                };

                template <typename Iter>
                void MakeStopsEdges(BusStopsData<Iter> routedata, Graph& graph) {
                    for (auto from_iter = routedata.first; from_iter != routedata.last; from_iter++) {
                        assert(*from_iter);

                        int span_count = 0;
                        double accumulated_weight = 0;
                        //in the two-vertexes structure, the distance between vertex portal and hub is 1
                        auto from_portal = graph.GetVertexId((*from_iter) -> name);
                        assert(from_portal);

                        auto from_hub = *from_portal + 1;
                        graph.SetEdgePath(graph.AddEdge({*from_portal, from_hub, double(settings_.bus_wait_time)}),
                                        {routedata.busname, span_count});

                        auto prev_vertex = from_iter;
                        for (auto to_iter = std::next(from_iter); to_iter != routedata.last; to_iter++) {

                            assert(*to_iter);
                            
                            std::string_view to_vertex_name = (*to_iter) -> name;
                            auto to_portal = graph.GetVertexId(to_vertex_name);
                            assert(to_portal);

                            static const int METERS_PER_KILOMETER = 1000;
                            static const int MINUTES_PER_HOUR = 60;
                            
                            graph.SetEdgePath(graph.AddEdge({from_hub, *to_portal, accumulated_weight += 
                                                            (database_.GetDistance((*prev_vertex) -> name, to_vertex_name) / 
                                                            (settings_.bus_velocity * METERS_PER_KILOMETER / MINUTES_PER_HOUR))}),
                                                            {routedata.busname, ++span_count});
                                                            
                            prev_vertex = to_iter;       
                        }
                    }
                }

                void MakeBusesEdges(Graph& graph) {
                    for (const auto& bus : database_.GetActiveRoutes()) {
                        using Stops = std::vector<Stop>;

                        if (bus) {
                            const Stops& stops = bus -> stops;
                            MakeStopsEdges<Stops::const_iterator>({bus -> name, stops.begin(), stops.end()}, graph);
                            if (!(bus -> is_roundtrip)) {
                                MakeStopsEdges<Stops::const_reverse_iterator>({bus -> name, stops.rbegin(), stops.rend()}, graph);
                            }
                        }
                    }
                }

            private:
                const Database& database_;
                const domain::RouterSettings& settings_;
                
            };

        private:
            Graph graph_;
            Router router_;
        };
    } //namespace router
} //namespace catalogue 







