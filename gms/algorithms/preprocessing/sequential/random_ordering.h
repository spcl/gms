#pragma once

#include <vector>
#include <algorithm>
#include <cstdlib>    

#include "../general.h"

namespace RandomOrder
{
    // Note: For the same input this will deterministically generate the same ordering.
    template <class CGraph = CSRGraph, class Output = std::vector<NodeId>>
    void getRandomIdOrder(const CGraph &g, Output &ranking)
    {
        ranking.resize(g.num_nodes());
        for(NodeId i = 0; i < (NodeId)g.num_nodes(); i++)
        {
            ranking[i] = i;
        }

        std::srand(0);
        std::random_shuffle(ranking.begin(), ranking.end());
    }
}