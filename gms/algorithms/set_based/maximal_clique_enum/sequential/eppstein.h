#pragma once

#ifndef BRONKERBOSCHEPPSTEIN_H
#define BRONKERBOSCHEPPSTEIN_H

#include "../general.h"
#include <gms/algorithms/preprocessing/preprocessing.h>
#include "tomita.h"

namespace BkEppstein
{

template <class SGraph, class Set = typename SGraph::Set>
std::vector<Set> mceRoaring(const SGraph &graph)
{
#ifdef BK_COUNT
    BK_CLIQUE_COUNTER = 0; //initialize counter
#endif

    std::vector<Set> sol = {};
    std::vector<NodeId> ordering;
    PpParallel::getDegeneracyOrderingApproxSGraph<PpParallel::boundary_function::averageDegree, false>(graph, ordering);
    Set remaining = Set::Range(graph.num_nodes());
    Set visited = {};

    for (const auto v : ordering)
    {
        auto &neigh = graph.out_neigh(v);
        auto cand = neigh.intersect(remaining);
        auto fini = neigh.intersect(visited);
        auto Q = Set(v);

        BkTomita::expand(cand, fini, Q, sol, graph);

        visited.union_inplace(v);
        remaining.difference_inplace(v);
    }

    return sol;
}

template <class SGraph, class Set = typename SGraph::Set>
std::vector<Set> mce(const CSRGraph &graph)
{
    return mceRoaring(SGraph::FromCGraph(graph));
}

} // namespace BkEppstein

#endif /*BRONKERBOSCHEPPSTEIN_H*/