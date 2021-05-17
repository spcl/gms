#include "../test_helper.h"
#include <gms/algorithms/set_based/link_prediction/edge_sampler.h>
#include <gms/representations/graphs/set_graph.h>

// TODO test statistical distributions

using namespace GMS::LinkPrediction;

TEST(EdgeSampler, NecessaryConditionPrimary) {
    auto graph = SortedSetGraph::FromCGraph(loadGraphFromFile("smallRandom1.el"));
    EdgeSampler sampler(graph);

    std::mt19937_64 rnd;
    for (int it = 0; it < 1000; ++it) {
        auto e = sampler.sample(rnd);
        ASSERT_TRUE(graph.out_neigh(e.first).contains(e.second));
    }
}

TEST(EdgeSampler, NecessaryConditionComplement) {
    auto graph = SortedSetGraph::FromCGraph(loadGraphFromFile("smallRandom1.el"));
    EdgeSampler sampler(graph);

    int num_nodes = graph.num_nodes();
    std::mt19937_64 rnd;
    for (int it = 0; it < 1000; ++it) {
        auto e = sampler.sample_complement(rnd);
        ASSERT_TRUE(0 <= e.first && e.first < num_nodes);
        ASSERT_TRUE(0 <= e.second && e.second < num_nodes);
        ASSERT_FALSE(graph.out_neigh(e.first).contains(e.second));
    }
}