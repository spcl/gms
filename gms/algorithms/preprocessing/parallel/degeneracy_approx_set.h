#pragma once

#include "../general.h"
#include <cstdlib>
#include <omp.h>
#include <gms/third_party/fast_statistics.h>
#include <gms/third_party/fast_range.h>
#include "boundary_function.h"

namespace PpParallel
{

template<BoundaryFunction boundary, bool useRankFormat = false, class SGraph = RoaringGraph, class Output = std::vector<NodeId>>
void getDegeneracyOrderingApproxSGraph(const SGraph &graph, Output &res, const double epsilon = 0.001) {
    using Set = typename SGraph::Set;

    auto vSize = graph.num_nodes();
    NodeId counter = 0;
    res.resize(vSize); //Prepare Result
    
    //Prepare Counter and Working Set
    std::vector<int> degreeCounter(vSize);
    NodeId *vArray = new NodeId[vSize];
#pragma omp parallel for schedule(static, 16)
    for (int i = 0; i < vSize; i++) {
        degreeCounter[i] = graph.out_neigh(i).cardinality();
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

        Set X(start_index, mid);

#pragma omp parallel
        {
            //Add to result set
#pragma omp for schedule(static, 16) nowait
            for (int i = 0; i < mid; i++) {
                if constexpr (useRankFormat) 
                    res[start_index[i]] = counter + i; //Result in Rank-Format
                else
                    res[counter + i] = start_index[i]; //Result in Order-Format
            }

            //Reflect removing the vertices from the graph (PULL style)
#pragma omp for schedule(static, 16)
            for (int i = mid; i < remaining; i++) {
                auto v = start_index[i];
                degreeCounter[v] -= graph.out_neigh(v).intersect_count(X);
            }
        }

        start_index = middle_index;
        counter += mid;
    }
    delete[] vArray;
}

} // namespace PpParallel