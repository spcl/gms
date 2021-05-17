#pragma once

#ifndef CORENRVERIFIER_H
#define CORENRVERIFIER_H

#include "gms/algorithms/preprocessing/general.h"
#include "gms/algorithms/preprocessing/verifiers/degeneracy_verifier.h"

namespace CoreNumberEvaluator
{

/*
CoreNumberOfOrder: Is the core number of the given order
CoreNumber: Is the core number of the graph
relativeError: Relative approxmiation error (approx - real)/real
faultRate: What is the proportion of vertices in the order which breaks the ordering, i.e. which have a deg > core Num
relativeMeanDifference: What is the average relative difference between an outlier and the core Num
*/
struct CoreNumberInfo
{
    size_t coreNumberOfOrder;
    size_t coreNumber;
    double relativeError;
    double faultRate;
    double relativeMeanDifference;

    //Auxiliary function to get get average over mutliple runs
    void add(CoreNumberInfo other)
    {
        coreNumberOfOrder += other.coreNumberOfOrder;
        relativeError += other.relativeError;
        faultRate += other.faultRate;
        relativeMeanDifference += other.relativeMeanDifference;
    }

    //Auxiliary function to get get average over mutliple runs
    void scaleDown(int numberOfRuns)
    {
        coreNumberOfOrder = std::round((double)coreNumberOfOrder / (double)numberOfRuns);
        relativeError /= numberOfRuns;
        faultRate /= numberOfRuns;
        relativeMeanDifference /= numberOfRuns;
    }
};

//Convert rankFormat to orderFormat and vice versa
template<class Input = std::vector<NodeId>>
void switchOrderingFormatInPlace(Input &ordering)
{
    auto n = ordering.size();
    pvector<NodeId> temp(n);

#pragma omp parallel for
    for(NodeId v = 0; v < n; v++)
        temp[ordering[v]] = v;

#pragma omp parallel for
    for(NodeId i = 0; i < n; i++)
        ordering[i] = temp[i];
}

//Convert rankFormat to orderFormat and vice versa
template<class Input = std::vector<NodeId>, class Output = std::vector<NodeId>>
void switchOrderingFormat(const Input &ordering, Output &result)
{
    auto n = ordering.size();

#pragma omp parallel for
    for(NodeId v = 0; v < n; v++)
        result[ordering[v]] = v;
}

template<bool useRankFormat = false, class Input = std::vector<NodeId>>
CoreNumberInfo evaluateCoreNrAccuracy(const Input &order, const RoaringGraph &graph, size_t actualCoreNumber)
{
    if constexpr (useRankFormat)
    {
        std::vector<NodeId> orderFormat(graph.num_nodes());
        switchOrderingFormat(order, orderFormat);
        return evaluateCoreNrAccuracy<false>(orderFormat, graph, actualCoreNumber);
    }
    else {
        size_t coreNumber = actualCoreNumber;
        unsigned biggerThanCore = 0;
        unsigned int difAcc = 0;

        RoaringSet visited{};

        for (auto v : order)
        {
            auto deg = graph.out_neigh(v)
                        .difference(visited)
                        .cardinality();

            if (deg > actualCoreNumber)
            {
                coreNumber = std::max(coreNumber, deg);
                biggerThanCore++;
                difAcc += deg - actualCoreNumber;
            }

            visited.union_inplace(v);
        }

        return {
            coreNumber,
            actualCoreNumber,
            (coreNumber - actualCoreNumber) / (double)actualCoreNumber,
            (double)biggerThanCore / (double)graph.num_nodes(),
            (biggerThanCore == 0) ? 0 : ((double)difAcc / (double)biggerThanCore) / (double)actualCoreNumber};
    }
}

//Get The degeneracy/Core Number of an order
template<bool useRankFormat = false, class Input = std::vector<NodeId>>
size_t getCoreNumberOfOrder(const Input &order, const RoaringGraph &graph)
{   
    if constexpr (useRankFormat)
    {
        std::vector<NodeId> orderFormat(graph.num_nodes());
        switchOrderingFormat(order, orderFormat);
        return getCoreNumberOfOrder<false>(orderFormat, graph);
    }
    else
    {
        RoaringSet visited{};
        size_t coreNumber = 0;
        for (auto v : order)
        {
            auto deg = graph.out_neigh(v)
                        .difference(visited)
                        .cardinality();

            coreNumber = std::max(coreNumber, deg);
            visited.union_inplace(v);
        }
        return coreNumber;
    }
}

template<bool useRankFormat = false, class Input = std::vector<NodeId>>
CoreNumberInfo evaluateCoreNrAccuracy(const Input &order, const RoaringGraph &graph)
{
    std::vector<NodeId> matulaDegeneracyOrder(graph.num_nodes());
    PpSequential::getDegeneracyOrderingMatula<RoaringGraph, false>(graph, matulaDegeneracyOrder);
    if constexpr(useRankFormat)
    {
        std::vector<NodeId> orderFormat(graph.num_nodes());
        switchOrderingFormat(order, orderFormat);
        return evaluateCoreNrAccuracy<false>(orderFormat, graph, getCoreNumberOfOrder(matulaDegeneracyOrder, graph));
    }
    else
        return evaluateCoreNrAccuracy<false>(order, graph, getCoreNumberOfOrder(matulaDegeneracyOrder, graph));
}

template<bool useRankFormat = false, class Input = std::vector<NodeId>>
CoreNumberInfo evaluateCoreNrAccuracy(const Input &order, const CSRGraph &graph)
{
    RoaringGraph rgraph = RoaringGraph::FromCGraph(graph);
    return evaluateCoreNrAccuracy<useRankFormat>(order, rgraph);
}

} // namespace CoreNumberVerifier

#endif