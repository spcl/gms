#include <iostream>
#include "gms/third_party/gapbs/gapbs.h"
#include <gms/algorithms/preprocessing/preprocessing.h>
#include "coloring_barenboim.h"
#include "coloring_dense_sparse.h"
#include "coloring_elkin.h"
#include "coloring_johansson.h"
#include "coloring_jones_v1.h"
#include "coloring_jones_v2.h"
#include "coloring_jones_v3.h"
#include "coloring_jones_v4.h"
#include <gms/common/cli/cli.h>

using namespace GMS;
using namespace GMS::Coloring;

CSRGraph Preprocess(const CSRGraph& g)
{
    std::vector<GMS::NodeId> ranking;
    PpSequential::getSimpleIdOrdering(g, ranking);

    return PpSequential::InduceDirectedGraph<>(g, ranking);
}

std::vector<NodeId> Reorder(const CSRGraph& g)
{
    std::vector<GMS::NodeId> ranking;
    PpSequential::getSimpleIdOrdering(g, ranking);
    return ranking;
}

template<typename GraphT_, typename GAPBSFunc, typename PPFunc, typename VerifierFunc, typename...PrintInfos>
void benchmarkGraphColoringWithPreprocess(const CLI::Args &args, GraphT_ &g, PPFunc preprocess, GAPBSFunc graph_coloring_f, VerifierFunc verify, PrintInfos... printInfos) {
    g.PrintStats();

    double total_seconds = 0;
    double pp_total_seconds = 0;
    int64_t total_colors = 0;
    Timer trial_timer;
    Printer printer;

    for (int i = 0; i < args.num_trials; i++) {
        std::vector<int32_t> coloring(g.num_nodes(), 0);

        trial_timer.Start();
        auto ppG = preprocess(g);
        trial_timer.Stop();
        PrintTime("Preprocess Time", trial_timer.Seconds());
        const double preprocTime = trial_timer.Seconds();
        printer << preprocTime;
        pp_total_seconds += preprocTime;

        trial_timer.Start();
        graph_coloring_f(g, coloring);  // coloring
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        printer << trial_timer.Seconds();

        total_seconds += trial_timer.Seconds();

        int nCol = uniqueColorsCount(coloring);
        total_colors += nCol;

        PrintLabel("Colors", std::to_string(nCol));
        printer << nCol;

        if (args.verify) {
            std::string passMark = verify(g, coloring, nCol) ? "PASS" : "FAIL";
            PrintLabel("Verification", passMark);  // verify
            printer << passMark;
        }

        printer.Enqueue(printInfos...);
        std::cout << printer << std::endl;
    }

    PrintTime("Average Time", total_seconds / args.num_trials);
    PrintTime("Average pp Time", pp_total_seconds/ args.num_trials);
    PrintTime("Average colors", (total_colors * 1.0)/args.num_trials);
}

template<typename GraphT_, typename GAPBSFunc, typename PPFunc, typename VerifierFunc, typename...PrintInfos>
void benchmarkGraphColoringWithReordering(const CLI::Args &args, GraphT_ &g, PPFunc reordering, GAPBSFunc graph_coloring_f, VerifierFunc verify, PrintInfos...printInfos) {
    g.PrintStats();

    double total_seconds = 0;
    double pp_total_seconds = 0;
    int64_t total_colors = 0;
    Timer trial_timer;
    Printer  printer;

    for (int i = 0; i < args.num_trials; i++) {
        std::vector<int32_t> coloring(g.num_nodes(), 0);

        trial_timer.Start();
        auto order = reordering(g);
        trial_timer.Stop();
        PrintTime("Preprocess Time", trial_timer.Seconds());
        const double preprocTime = trial_timer.Seconds();
        printer << preprocTime;
        pp_total_seconds += preprocTime;

        trial_timer.Start();
        graph_coloring_f(g, coloring, order);  // coloring
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        printer << trial_timer.Seconds();
        total_seconds += trial_timer.Seconds();

        int nCol = uniqueColorsCount(coloring);
        total_colors += nCol;

        PrintLabel("Colors", std::to_string(nCol));
        printer << nCol;

        if (args.verify) {
            std::string passMark = verify(g, coloring, nCol) ? "PASS" : "FAIL";
            PrintLabel("Verification", passMark);  // verify
            printer << passMark;
        }

        printer.Enqueue(printInfos...);
        std::cout << printer << std::endl;
    }

    PrintTime("Average Time", total_seconds / args.num_trials);
    PrintTime("Average pp Time", pp_total_seconds/ args.num_trials);
    PrintTime("Average colors", (total_colors * 1.0)/args.num_trials);
}

int main(int argc, char *argv[]) {
    CLI::Parser parser;
    auto [args, g] = parser.parse_and_load(argc, argv);

    using CGraph = CSRGraph;

    benchmarkGraphColoringWithPreprocess(args, g, Preprocess, coloring_barenboim_direct_interface<CGraph>, GCVerifierDeltaPlusOne<CGraph>, "coloring", "barenboim", "pp:simpleId");

    benchmarkGraphColoringWithPreprocess(args, g, Preprocess, graph_coloring_dense_sparse<CGraph>, GCVerifierWeak<CGraph>, "coloring", "dense/sparse", "pp:simpleId");

    benchmarkGraphColoringWithPreprocess(args, g, Preprocess, coloring_elkin_direct_interface<CGraph>, GCVerifierDeltaPlusOne<CGraph>, "coloring", "elkin", "pp:simpleId");

    benchmarkGraphColoringWithPreprocess(args, g, Preprocess, graph_coloring_johansson<CGraph>, GCVerifierDeltaPlusOne<CGraph>, "coloring", "johansson", "pp:simpleId");

    benchmarkGraphColoringWithReordering(args, g, Reorder, JonesV1::graph_coloring_jones<CGraph>, GCVerifierMaxColor<CGraph>, "coloring", "jones_v1", "pp:simpleId");

    benchmarkGraphColoringWithPreprocess(args, g, Preprocess, JonesV2::graph_coloring_jones<CGraph>, GCVerifierMaxColor<CGraph>, "coloring", "jones_v2", "pp:simpleId");

    benchmarkGraphColoringWithReordering(args, g, Reorder, JonesV3::graph_coloring_jones<CGraph>, GCVerifierMaxColor<CGraph>, "coloring", "jones_v3", "pp:simpleId");

    benchmarkGraphColoringWithReordering(args, g, Reorder, JonesV4::graph_coloring_jones<CGraph>, GCVerifierMaxColor<CGraph>, "coloring", "jones_v4", "pp:simpleId");
    return 0;
}
