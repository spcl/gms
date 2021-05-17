#pragma once
#include <gms/common/types.h>
#include <cassert>

namespace GMS::TriangleCount::Seq {

template<typename SGraph>
size_t count_total(const SGraph &graph) {
    size_t n = graph.num_nodes();

    size_t total = 0;
    for (NodeId u = 0; u < n; ++u) {
        const auto &neigh_u = graph.out_neigh(u);
        for (NodeId v : neigh_u) {
            if (u < v) {
                total += neigh_u.intersect_count(graph.out_neigh(v));
            }
        }
    }

    assert(total % 3 == 0);
    return total / 3;
}

}