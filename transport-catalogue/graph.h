#pragma once

#include "ranges.h"

#include <cstdlib>
#include <vector>
#include <utility>
#include <memory>
#include <optional>
#include <cassert>

namespace graph {

    using VertexId = size_t;
    using EdgeId = size_t;

    template <typename Weight>
    struct Edge {
        VertexId from;
        VertexId to;
        Weight weight;
    };

    template <typename Weight>
    class DirectedWeightedGraph {
    private:
        using IncidenceList = std::vector<EdgeId>;
        using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;

    public:
        DirectedWeightedGraph() = default;
        explicit DirectedWeightedGraph(size_t vertex_count);
        EdgeId AddEdge(Edge<Weight> edge);

        size_t GetVertexCount() const;
        size_t GetEdgeCount() const;
        const Edge<Weight>& GetEdge(EdgeId edge_id) const;
        IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

    private:
        std::vector<Edge<Weight>> edges_;
        std::vector<IncidenceList> incidence_lists_;
    };

    template <typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
        : incidence_lists_(vertex_count) {
    }

    template <typename Weight>
    EdgeId DirectedWeightedGraph<Weight>::AddEdge(Edge<Weight> edge) {
        edges_.push_back(edge);
        const EdgeId id = edges_.size() - 1;
        incidence_lists_.at(edge.from).push_back(id);
        return id;
    }

    template <typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
        return incidence_lists_.size();
    }

    template <typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
        return edges_.size();
    }

    template <typename Weight>
    const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
        return edges_.at(edge_id);
    }

    template <typename Weight>
    typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
    DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
        return ranges::AsRange(incidence_lists_.at(vertex));
    }


    template <typename Weight, typename Vertex>
    class DoubleVertexGraph : public DirectedWeightedGraph<Weight> {
    public: 
        static VertexId SingleToDoubleVertexPos(VertexId vertex_id);
        static VertexId DoubleToSingleVertexPos(VertexId vertex_id);

    public:
        struct EdgeSegmentInfo {
            std::string name; 
            int span_count;
            Weight weight;
                 

            EdgeSegmentInfo& SetName(std::string value) {
                name = std::move(value);
                return *this;
            }      

            EdgeSegmentInfo& SetSpanCount(int value) {
                span_count = value;
                return *this;
            }

            EdgeSegmentInfo& SetWeight(Weight value) {
                weight = value;
                return *this;
            }     

        };

        struct EdgePath {
            std::string_view path_name;
            int span_count;
        };

        DoubleVertexGraph(std::vector<Vertex> vertexes);
        void SetEdgePath(EdgeId edge, EdgePath path);
        const Vertex* GetVertex(VertexId double_vertex_id) const;
        const VertexId* GetVertexId(std::string_view vertex) const;
        EdgeSegmentInfo GetEdgeSegmentInfo(EdgeId edge_id) const;

    private:
        std::vector<Vertex> single_vertexes_;
        std::unordered_map<EdgeId, EdgePath> edge_to_path_;
        std::unordered_map<std::string_view, VertexId> vertexname_to_double_vertex_id_;
    }; 

    template <typename Weight, typename Vertex>
    DoubleVertexGraph<Weight, Vertex>::DoubleVertexGraph(std::vector<Vertex> vertexes)  
    : graph::DirectedWeightedGraph<Weight>(SingleToDoubleVertexPos(vertexes.size())) 
    , single_vertexes_(std::move(vertexes)) {

        if (!single_vertexes_.empty()) {
            //build vertexname_to_double_vertex_id_
            for (VertexId id = 0; id < single_vertexes_.size(); id++) {
                if (const Vertex& vertex = single_vertexes_[id]; vertex) {
                    vertexname_to_double_vertex_id_[vertex -> name] = SingleToDoubleVertexPos(id);
                }
            }
        }

        
    }

    template <typename Weight, typename Vertex>
    VertexId DoubleVertexGraph<Weight, Vertex>::SingleToDoubleVertexPos(VertexId vertex_id) {
        return vertex_id * 2;
    }

    template <typename Weight, typename Vertex>
    VertexId DoubleVertexGraph<Weight, Vertex>::DoubleToSingleVertexPos(VertexId vertex_id) {
        return vertex_id / 2;
    }

    template <typename Weight, typename Vertex>
    void DoubleVertexGraph<Weight, Vertex>::SetEdgePath(EdgeId edge, EdgePath path) {
        edge_to_path_[edge] = std::move(path);
    }

    template <typename Weight, typename Vertex>
    const Vertex* DoubleVertexGraph<Weight, Vertex>::GetVertex(VertexId double_vertex_id) const {
        auto id = DoubleToSingleVertexPos(double_vertex_id); 
        return id < single_vertexes_.size() ? &(single_vertexes_.at(id)) : nullptr;
    }

    template <typename Weight, typename Vertex>
    const VertexId* DoubleVertexGraph<Weight, Vertex>::GetVertexId(std::string_view vertex) const {
        auto result = vertexname_to_double_vertex_id_.find(vertex);
        return result != vertexname_to_double_vertex_id_.end() ? &(result -> second) : nullptr;
    }

    template <typename Weight, typename Vertex>
    typename DoubleVertexGraph<Weight, Vertex>::EdgeSegmentInfo 
    DoubleVertexGraph<Weight, Vertex>::GetEdgeSegmentInfo(EdgeId edge_id) const {
        auto edge = this -> GetEdge(edge_id);
        auto edge_path = edge_to_path_.at(edge_id);

        auto vertex = GetVertex(edge.from);
        assert(vertex);

        return EdgeSegmentInfo{}.SetName(edge_path.span_count > 0 ? std::string(edge_path.path_name) 
                                        : (*vertex) -> name)
                                .SetWeight(edge.weight)
                                .SetSpanCount(edge_path.span_count);
    }



} // namespace graph