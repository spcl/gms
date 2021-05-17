#pragma once

#include <gms/common/types.h>
#include <gms/algorithms/set_based/triangle_count/parallel/vertex.h>

namespace PpParallel {

    template<class SGraph>
    using TriangleCountFn_t = void (*)(const SGraph &graph, pvector<int64_t> &counts);

    template<class SGraph, TriangleCountFn_t<SGraph> CountFn = GMS::TriangleCount::Par::vertex_count2_once, class Output = std::vector<NodeId>>
    void triangleCountOrdering(const SGraph &graph, Output &ordering) {
        int64_t num_nodes = graph.num_nodes();
        ordering.resize(num_nodes);

        pvector<int64_t> counts(num_nodes);
        CountFn(graph, counts);

        for (NodeId u = 0; u < num_nodes; ++u) {
            ordering[u] = u;
        }

        auto compare = [&](int64_t u, int64_t v) { return counts[u] < counts[v]; };

    #ifdef _PARALLEL_ALGORITHM
        __gnu_parallel::sort(ordering.begin(), ordering.end(), compare);
    #else
        std::sort(ordering.begin(), ordering.end(), compare);
    #endif
    }

}