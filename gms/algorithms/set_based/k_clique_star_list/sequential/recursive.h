#pragma once

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <vector>
#include <random>

#include <gms/common/format.h>
#include "output.h"

/**
 * Sequential set-based implementation of the k-clique-star algorithm [1].
 *
 * [1]: https://doi.org/10.1007/978-3-030-01768-2_13
 */
namespace GMS::KCliqueStar::Seq {
    template <class Set, OutputMode mode>
    using Output = ListOutput<Set, mode, 2>;

    /**
     *
     * @tparam SGraph
     * @tparam TOutput
     * @param g         Input graph
     * @param k         How many elements are still missing to be added to curClique.
     * @param curClique The current partial k-clique (centroid of k-clique-star).
     *                  Note: It wouldn't have to be mutable but it is so that a copy can be avoided in recursion.
     * @param isect     All common neighbors of the nodes in curClique.
     * @param output
     */
    template <class SGraph, class TOutput>
    void RecursiveStepCliqueStar(const SGraph &g, const int32_t k,
                                 typename SGraph::Set &curClique,
                                 const typename SGraph::Set &isect,
                                 TOutput &output) {
        using Set = typename SGraph::Set;

        if (k == 0) {
            //curClique now holds a k-clique
            //k-star-clique adds every vertex v which is connected to all vertices in curClique
            //so now intersect all neighbors that are NOT in the k-star-clique

            auto vBegin = curClique.begin();
            Set kstarClique = g.out_neigh(*vBegin).difference(curClique);
            for (++vBegin; vBegin != curClique.end(); ++vBegin) {
                Set temp = g.out_neigh(*vBegin).difference(curClique);
                kstarClique.intersect_inplace(temp);
            }

            output.push({curClique.clone(), std::move(kstarClique)});
            return;
        }
        for (auto vi : isect) {
            Set cur_isect = isect.intersect(g.out_neigh(vi));
            bool correctOrder = true;
            for (auto vj : curClique) {
                if (vi <= vj) {
                    correctOrder = false;
                    break;
                }
            }
            if (correctOrder) {
                curClique.add(vi);
                RecursiveStepCliqueStar(g, k - 1, curClique, cur_isect, output);
                curClique.remove(vi);
            }
        }
    }

    template <class Set>
    using FinalOutput = std::vector<std::tuple<Set, Set>>;

    /**
     * Remove redundancy in the output by merging k-clique-stars for the same centroid.
     *
     * @tparam Set
     * @param redundant_output
     * @return
     */
    template <class Set>
    FinalOutput<Set> remove_redundancy(const Output<Set, OutputMode::List> &redundant_output) {
        FinalOutput<Set> output;
        output.reserve(redundant_output.size());

        // Convert the first element of every output value into a std::vector.
        for (const auto &[centroid, outer] : redundant_output) {
            std::vector<NodeId> values(centroid.cardinality());
            centroid.toArray(values.data());
            std::sort(values.begin(), values.end());
            //output.push_back(std::tuple<std::vector<NodeId>, Set>{std::move(values), std::move(outer)});
            output.push_back(std::make_tuple<std::vector<NodeId>, Set>(std::move(values), outer.clone()));
        }

        // Sort by centroid k-clique.
        using Tuple = std::tuple<std::vector<NodeId>, Set>;
        std::sort(output.begin(), output.end(), [](const Tuple &t1, const Tuple &t2) {
            std::get<0>(t1) <= std::get<0>(t2);
        });

        // Now merge all outer sets for the same k-clique.
        size_t idx_tgt = 0;
        for (size_t idx_src = 1; idx_src < output.size(); ++idx_src) {
            if (std::get<0>(output[idx_tgt]) == std::get<0>(output[idx_src])) {
                std::get<1>(output[idx_tgt]).union_inplace(std::get<1>(output[idx_src]));
                ++idx_src;
            } else {
                ++idx_tgt;
                output[idx_tgt] = std::move(output[idx_src]);
                ++idx_src;
            }
        }

        // Remove the trailing duplicates.
        output.resize(idx_tgt + 1);

        // Return the output.
        return output;
    }

    template<class SGraph, OutputMode TOutputMode>
    void CliqueStar(const SGraph &g, int32_t k, Output<typename SGraph::Set, TOutputMode> &output) {
        using Set = typename SGraph::Set;
        assert(k > 0);
        size_t num_nodes = g.num_nodes();

        for (NodeId u = 0; u < num_nodes; ++u) {
            Set curClique(u);
            RecursiveStepCliqueStar(g, k - 1, curClique, g.out_neigh(u), output);
        }
    }

    template <class SGraph>
    auto CliqueStarList(const SGraph &g, int32_t k) {
        using Set = typename SGraph::Set;
        Output<Set, OutputMode::List> output;
        CliqueStar(g, k, output);
        return output;
        //return remove_redundancy(output);
    }

}