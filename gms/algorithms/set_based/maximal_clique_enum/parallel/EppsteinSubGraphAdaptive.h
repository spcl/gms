#pragma once

#include "../general.h"
#include <gms/algorithms/preprocessing/preprocessing.h>


namespace BkEppsteinSubGraphAdaptive
{
    template <int Boundary, class SGraph, class Set = typename SGraph::Set>
    std::vector<Set> mceBench(const SGraph &rgraph, const pvector<NodeId> &ordering)
    {
#ifdef BK_COUNT
        BK_CLIQUE_COUNTER = 0; //initialize counter
#endif

        auto vCount = rgraph.num_nodes();
        std::vector<Set> sol = {};

#pragma omp parallel for schedule(dynamic) shared(rgraph, sol, ordering)
        for (int v = 0; v < vCount; v++)
        {
            auto &neigh = rgraph.out_neigh(v);
            Set cand = {};
            Set fini = {};
            Set Q(v);

            for (auto w : neigh)
            {
                if (ordering[w] > ordering[v])
                    cand.union_inplace(w);
                else
                    fini.union_inplace(w);
            }

            if (cand.cardinality() < Boundary)
                BkTomita::expand(cand, fini, Q, sol, rgraph);
            else
                BkEppsteinSubGraph::expandRelay(cand, fini, Q, sol, SGraphSubGraph(rgraph, v, cand, fini));
        }

        return sol;
    }

} // namespace BkEppsteinSubGraphAdaptive