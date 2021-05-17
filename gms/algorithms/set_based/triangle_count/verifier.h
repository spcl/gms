#pragma once

#include <vector>
#include <iostream>
#include <gms/third_party/gapbs/benchmark.h>
#include <gms/representations/graphs/set_graph.h>

namespace GMS::TriangleCount::Verify {

/**
 * Simple serial implementation for verification that uses std::set_intersection.
 */
template <class CGraph>
size_t compute_total_count(const CGraph &g) {
    size_t total = 0;
    std::vector<NodeId> intersection;
    intersection.reserve(g.num_nodes());
    for (NodeId u : g.vertices()) {
        for (NodeId v : g.out_neigh(u)) {
            auto new_end = std::set_intersection(g.out_neigh(u).begin(),
                                                 g.out_neigh(u).end(),
                                                 g.out_neigh(v).begin(),
                                                 g.out_neigh(v).end(),
                                                 intersection.begin());
            intersection.resize(new_end - intersection.begin());
            total += intersection.size();
        }
    }

    return total / 6;
}

/**
 * Verification using a simple serial implementation that uses std::set_intersection.
 */
bool total_count(const CSRGraph &g, size_t test_total) {
    size_t total = compute_total_count(g);
    if (total != test_total) {
        std::cout << total << " != " << test_total << std::endl;
    }
    return total == test_total;
}

template <int DivideBy = 1, class Output = std::vector<int64_t>>
bool vertex_count(const CSRGraph &graph, const Output &test_counts) {
    int64_t num_nodes = graph.num_nodes();

    int64_t total_test = 0;

    const RoaringGraph rgraph = RoaringGraph::FromCGraph(graph);
    for (NodeId u = 0; u < num_nodes; ++u) {
        int64_t count = 0;
        const auto &neigh_u = rgraph.out_neigh(u);
        for (NodeId v : neigh_u) {
            count += neigh_u.intersect_count(rgraph.out_neigh(v));
        }

        int64_t test_count = test_counts[u] / DivideBy;
        assert(count % 2 == 0);
        count /= 2;

        total_test += test_count;

        auto print_error = [&](){
            std::cerr << "vertex_count of node u " << u
                      << ", expected (divided) = " << count
                      << ", actual (divided)   = " << test_count
                      << ", actual (undivided) = " << test_counts[u]
                      << std::endl;
        };

        if (test_counts[u] % DivideBy != 0) {
            print_error();
            return false;
        }

        if (count != test_count) {
            print_error();
            return false;
        }
    }

    int64_t total_true = compute_total_count(graph);
    return total_true * 3 == total_test;
}
}