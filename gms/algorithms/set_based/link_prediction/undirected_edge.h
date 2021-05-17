#pragma once
#include <gms/common/types.h>
#include <vector>
#include <cassert>

namespace GMS::LinkPrediction {

/**
 * Represents an undirected edge as a std::pair, with the invariant first <= second.
 */
class UndirectedEdge : public std::pair<NodeId, NodeId>
{
public:
    UndirectedEdge() : std::pair<NodeId, NodeId>(0, 0) {}
    UndirectedEdge(NodeId u, NodeId v) : std::pair<NodeId, NodeId>(u, v) {
        assert(u <= v);
    }
};

// NOTE: Currently only used for debug assertions.
template <class SGraph>
int64_t count_undirected_edges(const SGraph &graph) {
    int64_t count = 0;
    int64_t num_nodes = graph.num_nodes();
    int64_t self_cycles = 0;

#pragma omp parallel for reduction(+: count, self_cycles)
    for (NodeId u = 0; u < num_nodes; ++u) {
        for (NodeId v : graph.out_neigh(u)) {
            if (u < v) {
                ++count;
                assert(graph.out_neigh(v).contains(u));
            } else if (u == v) {
                ++self_cycles;
            }
        }
    }

    return count + self_cycles / 2;
}

}