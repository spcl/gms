#include <gms/common/types.h>
#include <gms/common/benchmark.h>
#include <gms/algorithms/preprocessing/preprocessing.h>
#include "clique_counting.h"
#include "bench_helper.h"

using namespace GMS;
using namespace GMS::KClique;

template <class CGraph>
CGraph Preprocess(const CGraph& g, const CLApp& cli)
{
    std::vector<NodeId> ranking;
    PpSequential::getDegeneracyOrderingDanischHeap(g, ranking);
    return PpSequential::InduceDirectedGraph<>(g, ranking);
}

int main(int argc, char *argv[])
{
    auto [cli, g] = parse(argc, argv);

    using CGraph = CSRGraph;
    BenchmarkKernelPP(cli, g, Preprocess<CGraph>, Seq::Kclisting<CGraph>, Verifiers::Standard, "serial", "degeneracy");

    return 0;
}