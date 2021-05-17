#ifndef COLORING_COMMON_H_
#define COLORING_COMMON_H_

#include <omp.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"
#include "random_select.h"

#include <gms/common/types.h>
#include <gms/common/printer.h>
#include <gms/common/detail_timer.h>

namespace GMS::Coloring {

typedef int32_t ColorID;

// ---------------- Coloring Verifiers ---------------------------------

template <class CGraph>
bool GCVerifierWeak(const CGraph &g, const std::vector<int32_t> &coloring, const int32_t nColor) {
    // verifies color if >0, counts colored and correctly colored
    // still fails if not all vertices are correctly colored
    (void) nColor; // Unused argument
    const int64_t n = g.num_nodes();
    bool isCorrect = true;
    int64_t colored = 0;
    int64_t correct = 0;

    int64_t delta = 0;
#pragma omp parallel for reduction(max : delta)
    for (NodeId v = 0; v < n; v++) {
        if (delta < g.out_degree(v)) {
            delta = g.out_degree(v);
        }
    }

#pragma omp parallel for reduction(&& : isCorrect) reduction(+:colored) reduction(+:correct)
    for (NodeId v = 0; v < n; v++) {
        ColorID c = coloring[v];
        if(c!=0) {
            colored++;
        }
        if (c<=0 || c > delta+1) {
            isCorrect = false;
            if(c!=0) {
                std::cout << "color out of bounds " << v << " " << c << std::endl;
            }
        } else {
            bool conflict = false;
            for (NodeId u : g.out_neigh(v)) {
                int32_t c2 = coloring[u];
                if (c2>0 && c == c2) {
                    isCorrect = false;
                    conflict = true;
                    break;
                }
            }
            if(!conflict) {
                correct++;
            } else {
                std::cout << "conflict " << v << std::endl;
            }
        }
    }
    std::cout << "Weak verifier, colored: " << colored << ", correct: " << correct << std::endl;
    return isCorrect;
}

template <class CGraph>
bool GCVerifierDegree(const CGraph &g, const std::vector<int32_t> &coloring, const int32_t nColor) {
    (void) nColor; // Suppress unused variable warning
    const int64_t n = g.num_nodes();
    bool isCorrect = true;

#pragma omp parallel for reduction(&& : isCorrect)
    for (NodeId v = 0; v < n; v++) {
        int32_t c = coloring[v];
        if (c == 0 || c > g.out_degree(v) + 1) {
            isCorrect = false;
        } else {
            for (NodeId u : g.out_neigh(v)) {
                int32_t c2 = coloring[u];
                if (c == c2) {
                    isCorrect = false;
                    break;
                }
            }
        }
    }
    return isCorrect;
}

template <class CGraph>
bool GCVerifierMaxColor(const CGraph &g, const std::vector<int32_t> &coloring, const int32_t maxColor) {
    const int64_t n = g.num_nodes();
    bool isCorrect = true;

#pragma omp parallel for reduction(&& : isCorrect)
    for (NodeId v = 0; v < n; v++) {
        int32_t c = coloring[v];
        if (c == 0 || c > maxColor) {
            isCorrect = false;
        } else {
            for (NodeId u : g.out_neigh(v)) {
                int32_t c2 = coloring[u];
                if (c == c2) {
                    isCorrect = false;
                    break;
                }
            }
        }
    }
    return isCorrect;
}

template <class CGraph>
bool GCVerifierDeltaPlusOne(const CGraph &g, const std::vector<int32_t> &coloring, const int32_t maxColor) {
    (void) maxColor; // Unused argument
    const int64_t n = g.num_nodes();
    bool isCorrect = true;

    int32_t delta = 0;

#pragma omp parallel for reduction(max : delta)
    for (NodeId v = 0; v < n; v++) {
        if (delta < g.out_degree(v)) {
            delta = g.out_degree(v);
        }
    }

#pragma omp parallel for reduction(&& : isCorrect)
    for (NodeId v = 0; v < n; v++) {
        int32_t c = coloring[v];
        if (c <= 0 || c > delta + 1) {
            isCorrect = false;
            // std::cout << "Color not in delta + 1! (" << c << ")" << std::endl;
        } else {
            for (NodeId u : g.out_neigh(v)) {
                int32_t c2 = coloring[u];
                if (c == c2) {
                    isCorrect = false;
                    // std::cout << "Vertex " << v << " and " << u << " have the same color and are connected!" << std::endl;
                    break;
                }
            }
        }
    }
    return isCorrect;
}

template <class CGraph>
bool GCVerifierDeltaPlusOneWeak(const CGraph &g, const std::vector<int32_t> &coloring, const int32_t nColor) {
    // verifies color if >0, counts colored and correctly colored
    // still fails if not all vertices are correctly colored
    (void) nColor; // Unused argument
    const int64_t n = g.num_nodes();
    bool isCorrect = true;

    int32_t delta = 0;

#pragma omp parallel for reduction(max : delta)
    for (NodeId v = 0; v < n; v++) {
        if (delta < g.out_degree(v)) {
            delta = g.out_degree(v);
        }
    }

    int64_t colored = 0;
    int64_t correct = 0;
#pragma omp parallel for reduction(&& : isCorrect) reduction(+:colored) reduction(+:correct)
    for (NodeId v = 0; v < n; v++) {
        ColorID c = coloring[v];
        if (c != 0 || c > delta + 1) {
            colored++;
        }
        if (c == 0) {
            isCorrect = false;
        } else {
            bool conflict = false;
            for (NodeId u : g.out_neigh(v)) {
                int32_t c2 = coloring[u];
                if (c2 > 0 && c == c2) {
                    isCorrect = false;
                    conflict = true;
                    break;
                }
            }
            if(!conflict) {
                correct++;
            }
        }
    }
    std::cout << "Weak verifier, colored: " << colored << ", correct: " << correct << std::endl;
    return isCorrect;
}

int uniqueColorsCount(std::vector<int32_t> &coloring) {
    std::vector<int32_t> unique_colors(coloring);
    std::sort(unique_colors.begin(), unique_colors.end());
    return std::unique(unique_colors.begin(), unique_colors.end()) - unique_colors.begin();
}

template <class CGraph>
void sort_graph_neighborhoods(CGraph& g) {
    const int64_t n = g.num_nodes();
    #pragma omp parallel for
    for(NodeId v = 0; v < n; v++) {
        std::sort(g.out_neigh(v).begin(), g.out_neigh(v).end());
    }
}

template <class CGraph>
bool is_sorted(CGraph& g) {
    const int64_t n = g.num_nodes();
    bool isSorted = true;
    #pragma omp parallel for reduction(&& : isSorted)
    for(NodeId v = 0; v < n; v++) {
        NodeId lastU = 0;
        for(NodeId u : g.out_neigh(v)) {
            if(u<lastU) {isSorted = false;}
            lastU = u;
        }
    }
    return isSorted;
}



template<typename GraphT_, typename GAPBSFunc, typename PPFunc, typename VerifierFunc, typename...PrintInfos>
void benchmarkGraphColoringWithPreprocess(BenchCLApp cli, GraphT_ &g, PPFunc preprocess, GAPBSFunc graph_coloring_f, VerifierFunc verify, PrintInfos... printInfos) {
    g.PrintStats();

    double total_seconds = 0;
    double pp_total_seconds = 0;
    int64_t total_colors = 0;
    Timer trial_timer;
    Printer printer;

    for (int i = 0; i < cli.num_trials(); i++) {
        std::vector<int32_t> coloring(g.num_nodes(), 0);

        trial_timer.Start();
        auto ppG = preprocess(g, cli);
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

        if (cli.do_verify()) {
            std::string passMark = verify(g, coloring, nCol) ? "PASS" : "FAIL";
            PrintLabel("Verification", passMark);  // verify
            printer << passMark;
        }

        printer.Enqueue(printInfos...);
        std::cout << printer << std::endl;
    }

    PrintTime("Average Time", total_seconds / cli.num_trials());
    PrintTime("Average pp Time", pp_total_seconds/ cli.num_trials());
    PrintTime("Average colors", (total_colors * 1.0)/cli.num_trials());
}






template<typename GraphT_, typename GAPBSFunc, typename PPFunc, typename VerifierFunc, typename...PrintInfos>
void benchmarkGraphColoringWithReordering(BenchCLApp cli, GraphT_ &g, PPFunc reordering, GAPBSFunc graph_coloring_f, VerifierFunc verify, PrintInfos...printInfos) {
    g.PrintStats();

    double total_seconds = 0;
    double pp_total_seconds = 0;
    int64_t total_colors = 0;
    Timer trial_timer;
    Printer  printer;

    for (int i = 0; i < cli.num_trials(); i++) {
        std::vector<int32_t> coloring(g.num_nodes(), 0);

        trial_timer.Start();
        auto order = reordering(g, cli);
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

        if (cli.do_verify()) {
            std::string passMark = verify(g, coloring, nCol) ? "PASS" : "FAIL";
            PrintLabel("Verification", passMark);  // verify
            printer << passMark;
        }

        printer.Enqueue(printInfos...);
        std::cout << printer << std::endl;
    }

    PrintTime("Average Time", total_seconds / cli.num_trials());
    PrintTime("Average pp Time", pp_total_seconds/ cli.num_trials());
    PrintTime("Average colors", (total_colors * 1.0)/cli.num_trials());
}

} // namespace GMS::Coloring

#endif  // COLORING_COMMON_H_
