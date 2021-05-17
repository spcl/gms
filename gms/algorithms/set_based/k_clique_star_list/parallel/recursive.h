#pragma once

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <vector>
#include <random>

#include <gms/common/format.h>
#include "output.h"

/**
 * Parallel set-based implementation of the k-clique-star algorithm [1].
 *
 * [1]: https://doi.org/10.1007/978-3-030-01768-2_13
 */
namespace GMS::KCliqueStar::Par {
    template<class SGraph, OutputMode TOutputMode>
    void CliqueStar(const SGraph &g, int32_t k, Seq::Output<typename SGraph::Set, TOutputMode> &output) {
        assert(k > 0);
        size_t num_nodes = g.num_nodes();

        ListOutputPar<typename SGraph::Set, TOutputMode, 2> output_par;
        auto output_writer = output_par.writer();

#pragma omp parallel for schedule(dynamic, 64) firstprivate(output_writer)
        for (NodeId u = 0; u < num_nodes; ++u) {
            RoaringSet curClique(u);
            Seq::RecursiveStepCliqueStar(g, k - 1, curClique, g.out_neigh(u), output_writer);
        }

        output = std::move(output_par.collect());

        std::cout << "total " << k << "-cliques: " << output.size() << std::endl;
    }

    template <class SGraph>
    auto CliqueStarList(const SGraph &g, int32_t k) {
        Seq::Output<typename SGraph::Set, OutputMode::List> output;
        Par::CliqueStar<SGraph>(g, k, output);
        //return Seq::remove_redundancy(output);
        return output;
    }

}