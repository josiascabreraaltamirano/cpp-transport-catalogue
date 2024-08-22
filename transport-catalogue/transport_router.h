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

        class TransportRouterConstructor {
        public:
            TransportRouterConstructor& SetDataSource(const Database& source);
            TransportRouterConstructor& SetRouterSettings(const domain::RouterSettings& settings);
            const Database& GetDatabase() const;
            double GetVelocity() const;
            int GetWaitTime() const;

        private:
            static const int MINUTES_PER_HOUR = 60;
            static const int METERS_PER_KILOMETER = 1000;

            const Database* database_;
            //The velocity is saved in KM per minute
            double velocity_;
            //The wait_time is saved in minutes
            int wait_time_;
        };

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

            TransportRouter(TransportRouterConstructor data);
            std::optional<RoutePlan> BuildRoute(std::string_view from, std::string_view to) const;

        private:
            RoutePlan ProcessRouteInfo(const Router::RouteInfo& route_info) const;

            class TransportGraphFactory {
            public:
                TransportGraphFactory(const TransportRouterConstructor& data)
                : velocity_(data.GetVelocity())
                , wait_time_(data.GetWaitTime())
                , database_(data.GetDatabase())
                {  
                }

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
                        graph.SetEdgePath(graph.AddEdge({*from_portal, from_hub, double(wait_time_)}),
                                        {routedata.busname, span_count});

                        auto prev_vertex = from_iter;
                        for (auto to_iter = std::next(from_iter); to_iter != routedata.last; to_iter++) {

                            assert(*to_iter);
                            
                            std::string_view to_vertex_name = (*to_iter) -> name;
                            auto to_portal = graph.GetVertexId(to_vertex_name);
                            assert(to_portal);

                            graph.SetEdgePath(graph.AddEdge({from_hub, *to_portal, 
                                                            accumulated_weight += 
                                                            (database_.GetDistance((*prev_vertex) -> name, to_vertex_name) / velocity_)}),
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
                const double velocity_;
                const int wait_time_;
                const Database& database_;
            };

        private:
            Graph graph_;
            Router router_;
        };
    } //namespace router
} //namespace catalogue 







