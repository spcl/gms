#ifndef COLORING_SEQUENTIAL_H_
#define COLORING_SEQUENTIAL_H_
#include <omp.h>
#include <cassert>
#include <random>

#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/builder.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"

#include "coloring_common.h"
#include "random_select.h"

namespace GMS::Coloring {

template <class CGraph>
int graph_coloring_naive_sequential(const CGraph& g, std::vector<int32_t>& coloring) {
    int maxCol = 0;
    int64_t n = g.num_nodes();

    int64_t maxDeg = 0;
    for (NodeId v = 0; v < n; v++) {
        int64_t deg = g.out_degree(v);
        maxDeg = (maxDeg < deg) ? deg : maxDeg;
    }

    for (NodeId v = 0; v < n; v++) {
        std::vector<bool> palette(maxDeg + 2, false);
        for (NodeId u : g.out_neigh(v)) {
            int32_t c = coloring[u];
            palette[c] = true;
        }
        int32_t myC = 1;
        while (palette[myC]) {
            myC++;
        }
        coloring[v] = myC;
        maxCol = (maxCol < myC) ? myC : maxCol;
    }
    return maxCol;
}

} // namespace GMS::Coloring

#endif //COLORING_SEQUENTIAL_H_
