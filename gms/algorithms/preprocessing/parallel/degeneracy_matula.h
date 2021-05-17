#pragma once

#ifndef DEGORDERMATULAPAR_H
#define DEGORDERMATULAPAR_H

#include "../general.h"

namespace PpParallel
{
//In Place
template <class SGraph, bool useRankFormat = false, class Set = typename SGraph::Set, class Output = std::vector<NodeId>>
void getDegeneracyOrderingMatula(const SGraph &graph, Output &res)
{
    auto vSize = graph.num_nodes();
    Set L = {};
    Set V = Set::Range(vSize);
    std::vector<NodeId> d(vSize); // d[v] = deg(v)
    res.resize(vSize);
    //Compute Max Degree
    //Compute d[v]
    size_t maxDegree = 0;
#pragma omp parallel for schedule(static, 32) reduction(max : maxDegree)
    for (NodeId v = 0; v < vSize; v++)
    {
        auto deg = graph.out_neigh(v).cardinality();
        d[v] = deg;
        if (deg > maxDegree)
            maxDegree = deg;
    }

    //Prepare D, D[i] = all vertices v where N(v)-L = i
    std::vector<Set> D(maxDegree + 1);
    for (auto v : V)
        D[d[v]].union_inplace(v);

    unsigned int k = 0;
    for (int j = 0; j < vSize; j++)
    {
        unsigned int i = 0;
        while (D[i].cardinality() == 0 && i < (maxDegree + 1))
            i++;
        if (i == maxDegree + 1)
            break;
        k = (k >= i) ? k : i;
        auto v = *D[i].begin();

        if constexpr (useRankFormat) 
            res[v] = j; //Result in Rank-Format
        else
            res[j] = v; //Result in Order-Format

        L.union_inplace(v);
        D[i].difference_inplace(v);
        auto &neigh = graph.out_neigh(v);
        for (auto w : neigh)
        {
            if (!L.contains(w))
            {
                auto dw = d[w];
                d[w]--;
                D[dw].difference_inplace(w);
                D[dw - 1].union_inplace(w);
            }
        }
    }
}
} // namespace DegOrderMatulaPAR

#endif