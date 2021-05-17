#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/builder.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"
#include "gms/third_party/gapbs/platform_atomics.h"
#include "coloring_common.h"

#include <set>

namespace GMS::Coloring::JonesV3 {

int32_t get_color(NodeId v, std::vector<std::vector<NodeId>>& pred, std::vector<int32_t>& coloring) {
    std::vector<bool> colorMap(pred[v].size() + 1, false);

    // #pragma omp parallel for
    for (auto iter = pred[v].begin(); iter < pred[v].end(); iter++) {
        colorMap[coloring[*iter] - 1] = true;
    }

    return std::find(colorMap.begin(), colorMap.end(), false) - colorMap.begin() + 1;
}


void jp_color(NodeId v, std::vector<std::vector<NodeId>>& pred, std::vector<std::vector<NodeId>>& succ, std::vector<int64_t>& counter, std::vector<int32_t>& coloring) {
    coloring[v] = get_color(v, pred, coloring);

    // #pragma omp parallel for
    for (auto iter = succ[v].begin(); iter < succ[v].end(); iter++) {
        NodeId u = *iter;
        fetch_and_add(counter[u], -1);
        if (counter[u] == 0) {
            jp_color(u, pred, succ, counter, coloring);
        }
    }
}


template <class CGraph>
void graph_coloring_jones(const CGraph &g, std::vector<int32_t>& coloring, std::vector<NodeId> order) {

    int64_t numNodes = g.num_nodes();

    std::vector<std::vector<NodeId>> pred(numNodes);
    std::vector<std::vector<NodeId>> succ(numNodes);
    std::vector<int64_t> counter(numNodes);

    #pragma omp parallel for
    for (int64_t v=0; v<numNodes; v++) {
        int64_t predSize = 0;
        for (auto iter = g.out_neigh(v).begin(); iter < g.out_neigh(v).end(); iter++) {
            NodeId u = *iter;
            if (order[u] > order[v]) {
                pred[v].push_back(u);
                predSize++;
            } else {
                succ[v].push_back(u);
            }
        }
        counter[v] = predSize;
    }

    #pragma omp parallel for
    for (int v=0; v<numNodes; v++) {
        if (pred[v].size() == 0) {
            jp_color(v, pred, succ, counter, coloring);
        }
    }
}

} // namespace GMS::Coloring::JonesV3