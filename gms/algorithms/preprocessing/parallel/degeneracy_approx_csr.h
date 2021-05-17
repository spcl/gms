#pragma once

#include "../general.h"
#include <cstdlib>
#include <omp.h>
#include <gms/third_party/fast_statistics.h>
#include <gms/third_party/fast_range.h>
#include "boundary_function.h"

namespace PpParallel
{
template <BoundaryFunction boundary, bool useRankFormat = false, class CGraph = CSRGraph, class Output = std::vector<NodeId>, class Set = RoaringSet>
void getDegeneracyOrderingApproxCGraph(const CGraph &graph, Output &res, double epsilon)
{
    auto vSize = graph.num_nodes();
    NodeId counter = 0;
    res.resize(vSize); //Prepare Result
    
    //Prepare Counter and Working Set
    std::vector<int> degreeCounter(vSize);
    NodeId *vArray = new NodeId[vSize];
#pragma omp parallel for schedule(static, 16)
    for (int i = 0; i < vSize; i++) {
        degreeCounter[i] = graph.out_degree(i);
        vArray[i] = i;
    }

    NodeId *start_index = vArray;
    NodeId *end_index = vArray + vSize;

    while (counter < vSize) {
        auto remaining = end_index - start_index;

        unsigned int border = boundary(start_index, remaining, degreeCounter, epsilon);

        #ifdef _OPENMP
        auto middle_index = __gnu_parallel::partition(start_index, end_index,
                                                      [border, &degreeCounter](const NodeId v) {
                                                          return degreeCounter[v] <= border;
                                                      });
        #else
        auto middle_index = std::partition(start_index, end_index,
                                                      [border, &degreeCounter](const NodeId v) {
                                                          return degreeCounter[v] <= border;
                                                      });
        #endif
        
        auto mid = middle_index - start_index;

        #ifdef _OPENMP
        __gnu_parallel::sort(start_index, middle_index,
                             [&degreeCounter](const NodeId v, const NodeId w) { return degreeCounter[v] < degreeCounter[w]; });
        #else
        std::sort(start_index, middle_index,
                             [&degreeCounter](const NodeId v, const NodeId w) { return degreeCounter[v] < degreeCounter[w]; });
        #endif

        //Add to Result Set
        //And reflect removing the vertices from the graph (PUSH style)
#pragma omp parallel for schedule(static, 16)
        for (int i = 0; i < mid; i++) {
            if constexpr (useRankFormat) 
                res[start_index[i]] = counter + i; //Result in Rank-Format
            else
                res[counter + i] = start_index[i]; //Result in Order-Format
            auto v = start_index[i];
            for (auto u : graph.out_neigh(v))
            {
                #pragma omp atomic
                    degreeCounter[u]--;
            }
        }

        start_index = middle_index;
        counter += mid;
    }
    delete[] vArray;
}

} // namespace PpParallel
