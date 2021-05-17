#pragma once

#include <vector>

#include "../general.h"
#include "../util/auxiliary.h"
#include "../util/OrderedCollection.h"

namespace PpSequential
{
    template<typename Collection_T, typename Comparer_T, typename CGraph, typename Output>
    void getDegeneracyOrderingDanisch(const CGraph &g, Output &ranking)
    {
        typedef coreOrdering::KeyValuePair<NodeId, NodeId> KV_T;

        ranking.resize(g.num_nodes());

        std::vector<KV_T> kvList(g.num_nodes());
        for(NodeId i = 0; i < (NodeId)g.num_nodes(); i++)
        {
            kvList[i] = { i, (NodeId)g.out_degree(i) };
        }

        Collection_T orderedColl(kvList.data(), g.num_nodes());

        NodeId rcounter = 0;
        for(NodeId i = 0; i < g.num_nodes(); i++)
        {
            KV_T kv = orderedColl.PopHead();
            ranking[kv.Key] = g.num_nodes() - (++rcounter);
            std::vector<KV_T> neighbours(kv.Value);
            NodeId idx = 0;
            for(NodeId j : g.out_neigh(kv.Key))
            {
                // if node is still in subgraph
                if(orderedColl.GetIndex(j) != -1)
                {
                    neighbours[idx] = orderedColl.GetKeyValue(j);
                    idx++;
                }
            }
            auto cmp = Comparer_T();
            std::sort(neighbours.begin(), neighbours.end(), cmp);
            for(KV_T neighbour : neighbours)
            {
                orderedColl.DecreaseValueOfKey(neighbour.Key);
            }
        }
    }

    template <class CGraph = CSRGraph, class Output>
    void getDegeneracyOrderingDanischHeap(const CGraph &g, Output &ranking)
    {
        typedef coreOrdering::TrackingStdHeap<NodeId, NodeId> Sorting_T;
        getDegeneracyOrderingDanisch<Sorting_T, coreOrdering::NodeComparerMin>(g, ranking);
    }

    template <class CGraph = CSRGraph, class Output = pvector<NodeId>>
    void getDegeneracyOrderingDanischBubble(const CGraph &g, Output &ranking)
    {
        getDegeneracyOrderingDanisch<coreOrdering::TrackingBubblingArray<NodeId, NodeId>,
                coreOrdering::NodeComparerMax>(g, ranking);
    }
}