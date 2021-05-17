#pragma once

#ifndef BRONKERBOSCHEPPSTEINDEVSUBPAR_H
#define BRONKERBOSCHEPPSTEINDEVSUBPAR_H

#include "../general.h"
#include <gms/algorithms/preprocessing/preprocessing.h>

/* PARALLELIZED Eppstein using SubGraphs:*/
namespace BkEppsteinSubGraph
{

template <class SGraph, class Set>
NodeId findPivot(const Set &cand, const Set &fini, const SGraph &graph)
{
    NodeId pivot = *cand.begin();
    NodeId maxDeg = cand.intersect_count(graph.out_neigh(pivot));
    auto vPtr = cand.begin();
    auto end = cand.end();

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

template <class SubGraph, class Set>
void expand(Set &cand, Set &fini, Set &Q, std::vector<Set> &sol, const SubGraph &graph)
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

template <class SubGraph, class Set>
void expandRelay(Set &cand, Set &fini, Set &Q, std::vector<Set> &sol, const SubGraph &graph)
{

    if (cand.cardinality() != 0)
    {
        auto pivot = graph.findPivot(cand, fini);
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

        expandRelay(cand, fini, Q, sol, SGraphSubGraph<SGraph, Set>(rgraph, v, cand, fini));
    }

    return sol;
}

template <const auto Order, class SGraph, class Set = typename SGraph::Set>
std::vector<Set> mce(const SGraph &rgraph)
{
#ifdef BK_COUNT
    BK_CLIQUE_COUNTER = 0; //initialize counter
#endif

    auto vCount = rgraph.num_nodes();
    std::vector<Set> sol = {};
    pvector<NodeId> degOrder(vCount);
    Order(rgraph, degOrder);

    return mceBench(rgraph, degOrder);
}

} // namespace BkEppsteinSubGraph

#endif /*BRONKERBOSCHEPPSTEIN_H*/