#include <iostream>

#include "evaluation.h"
#include "link_prediction.h"

#include <gms/common/benchmark.h>
#include <gms/common/cli/cli.h>

using namespace GMS;
using namespace GMS::VertexSim;
using namespace GMS::LinkPrediction;


template <class SGraph, class CGraph>
std::tuple<SGraph, SGraph, size_t>
prepare_graphs(const CGraph &g, double test_rate) {
    assert(0.0 < test_rate && test_rate < 1.0);
    assert(!g.directed());
    SGraph g_train = SGraph::FromCGraph(g);
    const double num_edges = g.num_edges();
    SGraph g_test(g.num_nodes());
    size_t num_test_edges = test_rate * num_edges;
    std::cout << "extracting test edges" << std::endl;
    extract_random_test_edges(g_train, g_test, num_test_edges);
    std::cout << "extracted test edges" << std::endl;

    return std::tuple(std::move(g_train), std::move(g_test), num_test_edges);
}

template <class SGraph = RoaringGraph>
int bench_ranking(const CSRGraph &g, double test_rate) {
    auto [g_train, g_test, num_test_edges] = prepare_graphs<SGraph>(g, test_rate);

    // Find the 25% best test edges.
    std::cout << "prediction" << std::endl;
    auto scoring = link_prediction_similarity<Metric::Jaccard>(g_train, num_test_edges);
    //auto scoring = link_prediction_similarity<Similarity::Jaccard>(g_train, num_test_edges * 3);

    std::cout << "scoring" << std::endl;

    using EdgeSet = SortedSetBase<UndirectedEdge>;
    EdgeSet ES_scored(scoring.edges.data(), scoring.edges.size());

    auto score = score_link_prediction_precision(ES_scored, g_test);
    std::cout << "precision = " << score.precision << std::endl;
    std::cout << "recall    = " << score.recall << std::endl;

    return 0;
}

template <VertexSim::Metric TMetric, class SGraph>
double bench_auc(const CSRGraph &g, double test_rate, double false_link_rate, int64_t num_samples) {
    auto [g_train, g_test, num_test_edges] = prepare_graphs<SGraph>(g, test_rate);

    if (false_link_rate > 0.0) {
        // (1 - test_rate) * num_edges == number of non-test edges
        size_t num_false_links = false_link_rate * (1 - test_rate) * g.num_edges();
        add_false_links(g_train, num_false_links, g_test);
    }

    // Compute the AUC.
    std::cout << "scoring auc" << std::endl;
    double auc = score_link_prediction_auc<TMetric>(SGraph::FromCGraph(g), g_train, g_test, num_samples);
    std::cout << "auc = " << auc << std::endl;
    return auc;
}

constexpr auto bind_ranking(double test_rate) {
    return [=](const CSRGraph &g) {
        return bench_ranking(g, test_rate);
    };
}

template <VertexSim::Metric TMetric, class SGraph = SortedSetGraph>
constexpr auto bind_auc(double test_rate, double false_link_rate, int64_t num_samples) {
    return [=](const CSRGraph &g) {
        return bench_auc<TMetric, SGraph>(g, test_rate, false_link_rate, num_samples);
    };
}

int main(int argc, char *argv[]) {
    CLI::Parser parser;
    auto param_num_samples = parser.add_param("samples", std::nullopt, "100000", "Number of samples for AUC computation");
    auto [args, g] = parser.parse_and_load(argc, argv);

    int64_t num_samples = param_num_samples.to_int();

    BenchmarkKernel(args, g, bind_auc<Metric::Jaccard>(0.01, 0.0, num_samples), VerifyUnimplemented);
    BenchmarkKernel(args, g, bind_auc<Metric::Jaccard>(0.25, 0.01, num_samples), VerifyUnimplemented);
    BenchmarkKernel(args, g, bind_auc<Metric::Jaccard>(0.25, 0.05, num_samples), VerifyUnimplemented);

    BenchmarkKernel(args, g, bind_auc<Metric::AdamicAdar>(0.25, 0.0, num_samples), VerifyUnimplemented);
    BenchmarkKernel(args, g, bind_auc<Metric::CommNeigh>(0.25, 0.0, num_samples), VerifyUnimplemented);
    BenchmarkKernel(args, g, bind_auc<Metric::Overlap>(0.25, 0.0, num_samples), VerifyUnimplemented);
    BenchmarkKernel(args, g, bind_auc<Metric::PrefAtt>(0.25, 0.0, num_samples), VerifyUnimplemented);

    BenchmarkKernel(args, g, bind_ranking(0.25), VerifyUnimplemented);

    return 0;
}
