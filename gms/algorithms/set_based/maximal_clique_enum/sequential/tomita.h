#pragma once

#ifndef BRONKERBOSCHTOMITA_H
#define BRONKERBOSCHTOMITA_H

#include "../general.h"

namespace BkTomita
{

template <class SGraph, class Set>
NodeId findPivot(const Set &cand, const Set &fini, const SGraph &graph)
{
    auto vPtr = cand.begin();
    auto end = cand.end();
    NodeId pivot = *vPtr;
    NodeId maxDeg = cand.intersect_count(graph.out_neigh(pivot));

    for (vPtr++; vPtr != end; vPtr++)
    {
        auto v = *vPtr;
        auto deg = cand.intersect_count(graph.out_neigh(v));
        if (deg > maxDeg)
        {
            pivot = v;
            maxDeg = deg;
        }
    }
    for (auto v : fini)
    {
        auto deg = cand.intersect_count(graph.out_neigh(v));
        if (deg > maxDeg)
        {
            pivot = v;
            maxDeg = deg;
        }
    }

    return pivot;
}

/* DESCRIPTION:
subg:   cand u fini
cand:   set of vertices that can extend Q (==P)
fini:   set of vertices that have been used to extends Q (==X)
Q:      A clique to extend (==R)
sol:    Set of all maximal cliques
grap:   Input graph
*/
template <class SGraph, class Set>
void expand(Set &cand, Set &fini, Set &Q, std::vector<Set> &sol, const SGraph &graph)
{
    if (cand.cardinality() != 0)
    {
        auto pivot = findPivot(cand, fini, graph);
        auto Extu = cand.difference(graph.out_neigh(pivot));

        for (auto q : Extu)
        {
            auto &qNeigh = graph.out_neigh(q);

            auto candNew = cand.intersect(qNeigh);
            auto finiNew = fini.intersect(qNeigh);
            Q.union_inplace(q);

            expand(candNew, finiNew, Q, sol, graph);

            cand.difference_inplace(q);
            fini.union_inplace(q);
            Q.difference_inplace(q);
        }
    }
    else if (fini.cardinality() == 0)
    {
#ifdef BK_COUNT
#pragma omp atomic
        BK_CLIQUE_COUNTER++;
#endif
#ifdef MINEBENCH_TEST
#pragma omp critical
        {
            sol.push_back(Q.clone());
        }
#endif
    }
}

template <class SGraph, class Set = typename SGraph::Set>
std::vector<Set> mceRoaring(const SGraph &graph)
{
#ifdef BK_COUNT
    BK_CLIQUE_COUNTER = 0; //initialize counter
#endif
    std::vector<Set> sol = {};
    auto cand = Set::Range(graph.num_nodes());
    auto fini = Set();
    auto Q = Set();
    expand(cand, fini, Q, sol, graph);

    return sol;
}

template <class SGraph, class Set = typename SGraph::Set>
std::vector<Set> mce(const CSRGraph &graph)
{
    return mceRoaring(SGraph::FromCGraph(graph));
}
} // namespace BkTomita

#endif /*BRONKERBOSCHSIMPLETOMITA_H*/