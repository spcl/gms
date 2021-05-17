#pragma once

#ifndef DEGREEORDERCSR_H
#define DEGREEORDERCSR_H

#include "../general.h"
#include <cstdlib>
#include <functional>

namespace PpParallel
{
    // Helper function.
    //
    // This code is generic over both CGraph and SGraph.
    template <class AnyGraph>
    inline bool compare_degree(const AnyGraph *graph, NodeId v, NodeId w)
    {
        int64_t degV = graph->out_degree(v);
        int64_t degW = graph->out_degree(w);

        return !(degV > degW || (degV == degW && v > w));
    }

    // This code is generic over both CGraph and SGraph.
    template <class AnyGraph, bool useRankFormat = false, class Output = std::vector<NodeId>>
    void getDegreeOrdering(const AnyGraph &graph, Output &res)
    {
        using namespace std::placeholders;

        auto n = graph.num_nodes();
        res.resize(n);

        if constexpr (useRankFormat) 
        {
            //Produce result in Rank-Format
            std::vector<NodeId> temp(n);
            std::iota(temp.begin(), temp.end(), 0);

#ifdef _OPENMP
            __gnu_parallel::sort(temp.begin(), temp.end(), std::bind(compare_degree<AnyGraph>, &graph, _1, _2));
#else
            std::sort(temp.begin(), temp.end(), std::bind(compare_degree<AnyGraph>, &graph, _1, _2));
#endif

#pragma omp parallel for schedule(static, 64)
            for (int i = 0; i < n; i++)
                res[temp[i]] = i;
        }
        else 
        {
            //Produce result in Order-Format
            std::iota(res.begin(), res.end(), 0);

#ifdef _OPENMP
            __gnu_parallel::sort(res.begin(), res.end(), std::bind(compare_degree<AnyGraph>, &graph, _1, _2));
#else
            std::sort(res.begin(), res.end(), std::bind(compare_degree<AnyGraph>, &graph, _1, _2));
#endif
        }

    }
} // namespace PpParallel

#endif