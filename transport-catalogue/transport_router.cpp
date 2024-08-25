#include "transport_router.h"

#include <iostream>

namespace catalogue {
    namespace router {

        // TransportRouter public member functions definition

        TransportRouter::TransportRouter(const Database& source, const domain::RouterSettings& settings)
        : graph_(TransportRouter::TransportGraphFactory{source, settings}.MakeTransportGraph())
        , router_(graph_) 
        {
        }

        std::optional<TransportRouter::RoutePlan> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
            auto from_index = graph_.GetVertexId(from);
            auto to_index = graph_.GetVertexId(to);
            
            if (from_index && to_index) {
                auto result = router_.BuildRoute(*from_index, *to_index);
                if (result) {
                    return ProcessRouteInfo(*result);
                }
            }

            return {};
        } 

        //private class member functions
        TransportRouter::RoutePlan TransportRouter::ProcessRouteInfo(const Router::RouteInfo& route_info) const {
            std::vector<Graph::EdgeSegmentInfo> edges;
            for (auto edge_id : route_info.edges) {
                auto data = graph_.GetEdgeSegmentInfo(edge_id);
                edges.push_back(data);
                //std::cerr << data.path_name << " - " << data.edge -> from << " -> " << data.edge -> to << " = " << data.edge -> weight << std::endl;
            }
            return {route_info.weight, std::move(edges)};
        }    
    } // namespace router
} //namespace catalogue

