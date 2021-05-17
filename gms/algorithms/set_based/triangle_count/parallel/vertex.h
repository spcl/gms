#pragma once
#include <gms/common/types.h>

namespace GMS::TriangleCount::Par {

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
#pragma omp parallel for schedule(static, 9)
    for (NodeId u = 0; u < num_nodes; ++u) {
        int64_t count = 0;
        const auto &neigh_u = graph.out_neigh(u);
        for (NodeId v : neigh_u) {
            count += neigh_u.intersect_count(graph.out_neigh(v));
        }
        counts[u] = count;
    }
}

// This version only computes intersections once but still counts 2 times
template<class SGraph, class Output = std::vector<int64_t>>
void vertex_count2_once(const SGraph &graph, Output &counts) {
    int64_t num_nodes = graph.num_nodes();
    counts.resize(num_nodes);
#pragma omp parallel for schedule(dynamic, 9)
    for (NodeId u = 0; u < num_nodes; ++u) {
        int64_t count = 0;
        const auto &neigh_u = graph.out_neigh(u);
        for (NodeId v : neigh_u) {
            if (u < v) {
                int64_t c = neigh_u.intersect_count(graph.out_neigh(v));
                count += c;
#pragma omp atomic
                counts[v] += c;
            }
        }
#pragma omp atomic
        counts[u] += count;
    }
}

}