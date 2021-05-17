#pragma once

#ifndef BRONKERBOSCHEPPSTEINPAR_H
#define BRONKERBOSCHEPPSTEINPAR_H

#include "../general.h"
#include <ctime>
#include <gms/algorithms/preprocessing/preprocessing.h>
#include "../sequential/tomita.h"
#include <gms/third_party/fast_range.h>
#include <gms/third_party/fast_statistics.h>
#include <parallel/algorithm>
#include <gms/common/papi/papiw.h>

namespace BkEppsteinPar
{
    template <class SGraph, class Set = typename SGraph::Set>
    std::vector<Set> mceBench(const SGraph &rgraph, const pvector<NodeId> &ordering)
    {
#ifdef BK_COUNT
        BK_CLIQUE_COUNTER = 0; //initialize counter
#endif
        auto vCount = rgraph.num_nodes();
        std::vector<Set> sol = {};

        PAPIW::INIT_PARALLEL(PAPI_RES_STL, PAPI_TOT_CYC); // Init PAPIW

#pragma omp parallel shared(rgraph, sol, ordering)
        {
            PAPIW::START();
#pragma omp for schedule(dynamic)
            for (NodeId v = 0; v < vCount; v++)
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

                BkTomita::expand(cand, fini, Q, sol, rgraph);
            }
            PAPIW::STOP();
        }
        PAPIW::PRINT();
        return sol;
    }

    template <const auto Order, class SGraph, class Set = typename SGraph::Set>
    std::vector<Set> mce(const SGraph &rgraph)
    {
#ifdef BK_COUNT
        BK_CLIQUE_COUNTER = 0; //initialize counter
#endif

        pvector<NodeId> ordering(rgraph.num_nodes());
        Order(rgraph, ordering);

        return mceBench(rgraph, ordering);
    }

} // namespace BkEppsteinPar

#endif /*BRONKERBOSCHEPPSTEIN_H*/