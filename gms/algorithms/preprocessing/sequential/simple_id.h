#pragma once

#include <algorithm>
#include <vector>

#include "../general.h"

namespace PpSequential
{
    template <class CGraph = CSRGraph, class Output = std::vector<NodeId>>
    void getSimpleIdOrdering(const CGraph &g, Output &ranking) {
        ranking.resize(g.num_nodes());
        NodeId num_nodes = g.num_nodes();
        for (NodeId i = 0; i < num_nodes; ++i) {
            ranking[i] = i;
        }
    }
}