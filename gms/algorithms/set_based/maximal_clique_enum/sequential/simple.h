#pragma once

#ifndef BRONKERBOSCHSIMPLE_H
#define BRONKERBOSCHSIMPLE_H

#include "../general.h"

namespace BkSimple
{
//TODO: Using new local Variables since DISABLE_COPY restrict re assignments
//===> Confirm with Jakub that this doesn't harm performance
//template <typename Set, typename SGraph>
template <class SGraph, class Set = typename SGraph::Set>
void bronKerboschRec(Set R, Set P, Set X, std::vector<Set> &sol, const SGraph &graph)
{
    //NOTE: At the moment, this is the only place where in place operation is really needed
    if (P.cardinality() == 0 && X.cardinality() == 0)
    {
#ifdef BK_COUNT
#pragma omp atomic
        BK_CLIQUE_COUNTER++; //initialize counter
#endif
#ifdef MINEBENCH_TEST
        sol.push_back(std::move(R));
#endif
    }
    else
    {
        auto iterator = P.begin();
        while (!P.cardinality() == 0 && iterator != P.end())
        {
            auto v = *iterator;
            auto &neighs = graph.out_neigh(v);

            bronKerboschRec(R.union_with(v), P.intersect(neighs), X.intersect(neighs), sol, graph);

            P.difference_inplace(v);
            X.union_inplace(v);

            if (!P.cardinality() == 0)
            {
                iterator = P.begin();
            }
        }
    }
}

template <class SGraph, class Set = typename SGraph::Set>
std::vector<Set> mce(const CSRGraph &graph)
{
#ifdef BK_COUNT
    BK_CLIQUE_COUNTER = 0; //initialize counter
#endif
    size_t size = graph.num_nodes();
    std::vector<Set> sol = {};

    bronKerboschRec(Set(), Set::Range(size), Set(), sol, SGraph::FromCGraph(graph));

    return sol;
}
} // namespace BkSimple

#endif /*BRONKERBOSCHSIMPLE_H*/