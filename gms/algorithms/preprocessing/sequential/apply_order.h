#pragma once

#include <vector>

#include "gms/algorithms/preprocessing/general.h"

namespace PpSequential
{
template <class CGraph = CSRGraph>
CGraph InduceDirectedGraph(const CGraph& g, const std::vector<NodeId>& ranking)
{
    using Edge = EdgePair<NodeId, NodeId>;

    if (g.directed()) {
        throw std::invalid_argument("Graph must be undirected");
    }

    pvector<Edge> list(0);
    list.reserve(g.num_nodes());

    for (NodeId nodeIdx = 0; nodeIdx < g.num_nodes(); nodeIdx++) {
        NodeId source = ranking[nodeIdx];
        for (NodeId neighIdx : g.out_neigh(nodeIdx)) {
            NodeId target = ranking[neighIdx];
            if (source < target) {
                list.push_back(Edge(source, target));
            }
        }
    }

    CLApp cli(0, nullptr, "dummy");
    BuilderBase<NodeId, NodeId, NodeId> b(cli);

    return b.MakeGraphFromELGeneric<CGraph>(list);
}
} // namespace PpSequential