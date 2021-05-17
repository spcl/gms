#pragma once
#include <gms/common/types.h>

namespace GMS::TriangleCount::Seq {

/**
 * Computes 2 times the number of triangles for each vertex in the graph.
 *
 * @tparam SGraph
 * @tparam Output
 * @param graph
 * @param counts
 */
template<class SGraph, class Output = std::vector<int64_t>>
void vertex_count2(const SGraph &graph, Output &counts) {
    int64_t num_nodes = graph.num_nodes();
    counts.resize(num_nodes);
    for (NodeId u = 0; u < num_nodes; ++u) {
        int64_t count = 0;
        const auto &neigh_u = graph.out_neigh(u);
        for (NodeId v : neigh_u) {
            count += neigh_u.intersect_count(graph.out_neigh(v));
        }
        counts[u] = count;
    }
}

}