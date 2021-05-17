#pragma once

#include <gms/representations/graphs/set_graph.h>

template <class SGraph, class Set>
size_t RecursiveStepCliqueCount(SGraph& graph, const size_t k, const Set &isect) {
    if (k == 1)
        return isect.cardinality();
    assert(k > 1);
    size_t current = 0;
    for (auto vi : isect) {
        auto cur_isect = isect.intersect(graph.out_neigh(vi));
        if (cur_isect.cardinality() >= k - 2)
            current += RecursiveStepCliqueCount(graph, k - 1, cur_isect);
    }
    return current;
}

template <typename Set, typename SGraph, typename Set2>
size_t CliqueCount(CSRGraph &g, size_t k = 4) {
    size_t n = g.num_nodes();
    SGraph set_graph = SGraph::FromCGraph(g);
    size_t total = 0;

#pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
    for (NodeId u = 0; u < n; ++u){
        total += RecursiveStepCliqueCount(set_graph, k - 1, set_graph.out_neigh(u));
    }
    std::cout << "total " << k << "-cliques: " << total << std::endl;
    return total;
}