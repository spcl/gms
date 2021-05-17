#pragma once

#include <gms/common/cli/cli.h>
#include <gms/common/pipeline.h>
#include "clique_counting.h"

namespace GMS::KClique {

std::tuple<CLCliqueApp, CSRGraph> parse(int argc, char **argv) {
    GMS::CLI::Parser parser;
    auto clique_size = parser.add_param("clique-size", "cs", "8", "the clique size");
    auto [args, g] = parser.parse_and_load(argc, argv);
    return std::make_tuple<CLCliqueApp, CSRGraph>(CLCliqueApp(args, clique_size), std::move(g));
}

template <const bool TNodeParallel, class CGraph = CSRGraph>
class CliqueCountPipeline : public Pipeline
{
private:
    using KclistGraphT = std::conditional_t<TNodeParallel, NP::graph, EP::graph>;

public:
    const CLApp& clApp;
    CGraph *originalGraph;
    std::optional<CGraph> orderedGraph;
    KclistGraphT *danischGraph;
    unsigned long long count;
    double epsilon;

    CliqueCountPipeline(const CLApp& clapp) : clApp(clapp), originalGraph(nullptr), danischGraph(nullptr), count(0), epsilon(1.)
    {}

    void Preprocess()
    {
        std::vector<NodeId> ranking;
        PpSequential::getDegeneracyOrderingDanischHeap(*originalGraph, ranking);
        orderedGraph = PpSequential::InduceDirectedGraph<CGraph>(*originalGraph, ranking);
    }

    void PreprocessSimple()
    {
        std::vector<NodeId> ranking;
        PpSequential::getSimpleIdOrdering(*originalGraph, ranking);
        orderedGraph = PpSequential::InduceDirectedGraph<CGraph>(*originalGraph, ranking);
    }

    void PreprocessDegree()
    {
        std::vector<NodeId> ranking;
        PpSequential::getDegreeOrdering<CGraph>(*originalGraph, ranking);
        orderedGraph = PpSequential::InduceDirectedGraph<CGraph>(*originalGraph, ranking);
    }

    template<BoundaryFunction ApproxSorting_T, bool useRankFormat = false>
    void PreprocessApprox()
    {
        std::vector<NodeId> sortedVertices;
        PpParallel::getDegeneracyOrderingApproxCGraph<ApproxSorting_T, useRankFormat>(*originalGraph, sortedVertices, epsilon);
        std::vector<NodeId> ranking(originalGraph->num_nodes());
        for(NodeId i = 0; i < originalGraph->num_nodes(); i++)
        {
            ranking[sortedVertices[i]] = i;
        }
        orderedGraph = PpSequential::InduceDirectedGraph<CGraph>(*originalGraph, ranking);
    }

    void kclisting()
    {
        if constexpr (TNodeParallel) {
            count = Par::NP_kclisting<CGraph>(orderedGraph.value(), clApp);
        } else {
            count = Par::EP_kclisting<CGraph>(orderedGraph.value(), clApp);
        }
    }

    void verifierSetup()
    {
        if constexpr (TNodeParallel) {
            danischGraph = NP::ToGraph(orderedGraph.value());
        } else {
            danischGraph = EP::ToGraph(orderedGraph.value());
        }
    }

    void verify()
    {
        bool pass;
        if constexpr (TNodeParallel) {
            pass = Verifiers::NP(danischGraph, count, clApp);
        } else {
            pass = Verifiers::EP(danischGraph, count, clApp);
        }
        LocalPrinter << (pass? "pass" : "failed");
    }

    void verifierTearDown()
    {
        if constexpr (TNodeParallel) {
            Verifiers::NPTearDown(danischGraph, clApp);
        } else {
            Verifiers::EPTearDown(danischGraph, clApp);
        }
    }


};

}