#pragma once

#ifndef DEGENERACYVERIFIER_H
#define DEGENERACYVERIFIER_H

#include "../general.h"
#include "../sequential/degeneracy_matula.h"
#include "gms/algorithms/preprocessing/util/core_number_evaluator.h"

#include <gms/common/format.h>

namespace DegeneracyOrderingVerifier
{

bool graphIsNull(const RoaringGraph &graph)
{
    int size = graph.num_nodes();
    for (int v = 0; v < size; v++)
    {
        if (graph.out_neigh(v).cardinality() > 0)
            return false;
    }
    return true;
}

void eraseVertex(RoaringGraph &graph, NodeId v)
{
    auto V = RoaringSet::Range(graph.num_nodes());
    auto &neigh = graph.out_neigh(v);
    for (auto u : neigh)
    {
        graph.out_neigh(u).difference_inplace(v);
    }
    graph.out_neigh(v).difference_inplace(std::move(V)); //erase all Neighbours from v
}

//NOTE: This Function is just for testing
//Compute the degeneracy of a graph naivly
unsigned int getDegeneracy(RoaringGraph graph)
{

    auto remaining = RoaringSet::Range(graph.num_nodes());
    unsigned int deg = 0;
    do
    {
        deg++;
        bool changed = true;
        while (changed)
        {
            changed = false;
            RoaringSet toDelete = {};
            for (auto v : remaining)
            {
                if (graph.out_neigh(v).cardinality() < deg)
                {
                    eraseVertex(graph, v);
                    toDelete.union_inplace(v);
                    changed = true;
                }
            }
            if (changed)
                remaining.difference_inplace(toDelete);
        }

    } while (!graphIsNull(graph));
    return deg - 1;
}


bool degegeneracyOrderingVerifier(const CSRGraph &g, std::vector<NodeId> &result)
{
    if(g.num_nodes() != result.size())
        return false;

    const RoaringGraph rgraph = RoaringGraph::FromCGraph(g);
    auto degeneracy = getDegeneracy(RoaringGraph::FromCGraph(g));
    RoaringSet visited = {};
    bool correct = true;
    for (auto &v : result)
    {
        auto deg = rgraph.out_neigh(v).difference(visited).cardinality();
        correct = correct && (degeneracy >= deg);
        visited.union_inplace(v);
    }
    return correct;
}

template<class CGraph = CSRGraph, bool useRankFormat = false>
bool degeneracyOrderingApproxVerifier(const CGraph &g, std::vector<NodeId> &result)
{
    auto n = g.num_nodes();
    if(n != result.size())
        return false;

    //Check that all vertices are present in ordering
    RoaringSet visited = {};
    for (int v = 0; v < n; v++)
        visited.add(v);

    if(visited.cardinality() != g.num_nodes())
        return false;

    //Check that core Number of order is at least as good as degree ordering
    std::vector<NodeId> degreeOrdering(g.num_nodes());
    PpParallel::getDegreeOrdering(g, degreeOrdering);
    const RoaringGraph rgraph = RoaringGraph::FromCGraph(g);
    auto coreNumberDegree = CoreNumberEvaluator::getCoreNumberOfOrder(degreeOrdering, rgraph);
    auto coreNumberInput = CoreNumberEvaluator::getCoreNumberOfOrder<useRankFormat>(result, rgraph);
    if(coreNumberInput > coreNumberDegree)
        return false;

    return true;
}

template<bool useRankFormat = false>
bool degreeOrderingVerifier(const CSRGraph &g, std::vector<NodeId> &result)
{
    auto n = g.num_nodes();

    if constexpr (useRankFormat)
    {
        std::vector<NodeId> orderFormat(n);
        CoreNumberEvaluator::switchOrderingFormat(result, orderFormat);
        return degreeOrderingVerifier<false>(g, orderFormat);
    }

    if(n != result.size())
        return false;

    for(int i = 0; i < n-1; i++)
        if(g.out_degree(result[i]) > g.out_degree(result[i+1]))
            return false;

    return true;
}
} // namespace DegeneracyOrderingVerifier

#endif