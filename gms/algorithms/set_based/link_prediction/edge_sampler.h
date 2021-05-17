#pragma once

#include <vector>
#include <algorithm>
#include <functional>

#include <gms/common/types.h>
#include <gms/representations/sets/sorted_set.h>
#include <gms/third_party/fast_range.h>
#include "undirected_edge.h"

namespace GMS::LinkPrediction {

/**
 * @brief Provides sampling of undirected edges from a graph and its complement respectively.
 *
 * This class assumes an undirected and sparse graph.
 *
 * Several modifications might be necessary to make it work with a directed graph,
 * and the methods would have to be adjusted for a dense graph.
 *
 * @tparam SGraph The graph class to be sampled from.
 */
template <class SGraph>
class EdgeSampler {
    using Set = typename SGraph::Set;
public:
    EdgeSampler(const SGraph &graph, bool initialize_primary=true, bool initialize_complement=true) :
        graph(graph)
    {
        rebuild(initialize_primary, initialize_complement);
    }

    /**
     * Rebuild cumulative sums of neighborhood degrees.
     *
     * In most cases you should prefer the `rebuild()` method without arguments,
     * as this will already make sure that the correct sums are rebuilt.
     *
     * @param primary    whether to rebuild primary sums
     * @param complement whether to rebuild complement sums
     */
    void rebuild(bool primary, bool complement) {
        uint64_t num_nodes = graph.num_nodes();

        uint64_t num_edges = 0;
        uint64_t num_compl = 0;
        if (primary) { cumulative_degree.resize(num_nodes); }
        if (complement) { cumulative_complement_degree.resize(num_nodes); }

        for (uint64_t u = 0; u < num_nodes; ++u) {
            uint64_t c = graph.out_degree(u);
            if (primary) {
                num_edges += c;
                cumulative_degree[u] = num_edges;
            }
            if (complement) {
                assert(std::numeric_limits<uint64_t>::max() - (num_nodes - c) > num_compl);
                num_compl += num_nodes - c;
                cumulative_complement_degree[u] = num_compl;
            }
        }
    }

    /**
     * Rebuild cumulative sums of neighborhood degrees.
     *
     * This method should be called if the input graph is changed an further edges are to be sampled.
     */
    void rebuild() {
        rebuild(!cumulative_degree.empty(), !cumulative_complement_degree.empty());
    }

    /**
     * Sample a random edge of the graph uniformly.
     *
     * @tparam Rng STL compatible RNG class
     * @param rng the random number generator to be used
     * @return
     */
    template <class Rng>
    UndirectedEdge sample(Rng &rng) const {
        // Find the first vertex associated with this edge.
        auto [edge_index, u] = sample_weighted(cumulative_degree, rng);

        // Now find the second node of the edge.
        auto neigh = graph.out_neigh(u).begin();
        uint64_t offset = edge_index - ((u > 0) ? cumulative_degree[u - 1] : 0);
        std::advance(neigh, offset);
        NodeId v = *neigh;

        return UndirectedEdge(std::min(u, v), std::max(u, v));
    }

    /**
     * Sample a random edge of the complement graph uniformly, i.e. an edge which doesn't exist.
     *
     * @tparam Rng STL compatible RNG class
     * @param rng the random number generator to be used
     * @return
     */
    template <class Rng>
    UndirectedEdge sample_complement(Rng &rng) const {
        // Sample a complement edge.
        auto [edge_index, u] = sample_weighted(cumulative_complement_degree, rng);
        assert(u < graph.num_nodes());
#ifndef NDEBUG
        int64_t num_complement_u = graph.num_nodes() - graph.out_degree(u);
#endif
        assert(0 < num_complement_u);
        assert(num_complement_u < graph.num_nodes());

        // Now find the second node of the complement edge.
        uint64_t offset = edge_index - ((u > 0) ? cumulative_complement_degree[u - 1] : 0);
        assert(offset < num_complement_u);

        NodeId v = offset;
        if constexpr (std::is_same_v<Set, SortedSet>) {
            for (NodeId k : graph.out_neigh(u)) {
                if (k <= v) { v++; } else { break; }
            }
        } else {
            const auto &neigh = graph.out_neigh(u);
            std::vector<NodeId> indices(neigh.cardinality());
            std::sort(indices.begin(), indices.end());
            for (NodeId k : indices) {
                if (k <= v) { v++; } else { break; }
            }
        }

        auto edge = UndirectedEdge(std::min(u, v), std::max(u, v));
        if (graph.out_neigh(edge.first).contains(edge.second)) {
            const auto &neigh = graph.out_neigh(edge.first);
            neigh.contains(edge.second);
        }
        assert(!graph.out_neigh(edge.first).contains(edge.second));
        return edge;
    }

private:
    const SGraph &graph;

    std::vector<uint64_t> cumulative_degree;
    std::vector<uint64_t> cumulative_complement_degree;

    template <class Rng>
    std::tuple<uint64_t, NodeId> sample_weighted(const std::vector<uint64_t> &c_degree, Rng &rng) const {
        assert(c_degree.size() > 0);
        int64_t num_candidates = c_degree.back();
        assert(num_candidates > 0);
        uint64_t edge_index = fastrange64(rng(), num_candidates);
        auto iterator = std::upper_bound(c_degree.begin(), c_degree.end(), edge_index);
        return std::tuple(edge_index, std::distance(c_degree.begin(), iterator));
    }
};

}