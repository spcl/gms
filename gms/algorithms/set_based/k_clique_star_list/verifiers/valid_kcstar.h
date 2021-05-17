#pragma once

#include <gms/common/types.h>
#include <gms/common/format.h>
#include <gms/representations/graphs/set_graph.h>

namespace GMS::KCliqueStar::Verify {

/**
 * Verify if an individual k-clique-star is a valid k-clique-star.
 *
 * @param g the full input graph
 * @param kcstar the nodes of the k-clique-star to be checked
 * @param k size of the centroid k-clique
 * @return true if it is a valid k-clique-star, false otherwise
 */
bool KCliqueStarVerifier(const RoaringGraph &g, const RoaringSet &kcstar, int32_t k)
{
    std::vector<NodeId> nodes_not_kclique, nodes_yes_kclique;
    int64_t set_size = kcstar.cardinality();
    nodes_not_kclique.reserve(set_size);
    nodes_yes_kclique.reserve(set_size);

    for (NodeId u : kcstar) {
        int64_t self_cycle = g.out_neigh(u).contains(u);
        if (g.out_neigh(u).intersect_count(kcstar) == set_size - 1 + self_cycle) {
            nodes_yes_kclique.push_back(u);
        } else {
            nodes_not_kclique.push_back(u);
        }
    }

    if (nodes_yes_kclique.size() < k) {
        printArray("not_kcs", nodes_not_kclique);
        printArray("yes_kcs", nodes_yes_kclique);
        printSubgraphNeighborhoods(g, kcstar);

        return false;
    } else if (nodes_yes_kclique.size() > k) {
        // Its items are supposed to be connected to all outside nodes, so we can migrate `size() - k` to not_kclique.
        int remove_elements = nodes_yes_kclique.size() - k;
        for (int i = 0; i < remove_elements; ++i) {
            nodes_not_kclique.push_back(nodes_yes_kclique.back());
            nodes_yes_kclique.pop_back();
        }
    }

    RoaringSet kclique(nodes_yes_kclique);
    for (NodeId u : nodes_not_kclique) {
        if (g.out_neigh(u).intersect_count(kclique) != k) {
            std::cout << "u = " << u << std::endl;
            printSubgraphNeighborhoods(g, kclique.union_with(u));

            return false;
        }
    }

    return true;
}

/**
 * Verifies if all k-clique-stars are valid.
 *
 * Note: Doesn't validate that all have been found.
 *
 * @param cgraph
 * @param kcstars
 * @param k
 * @return
 */
template <class Output>
bool KCliqueStarsVerifier(const CSRGraph &cgraph, const Output &kcstars, int32_t k)
{
    RoaringGraph graph = RoaringGraph::FromCGraph(cgraph);
    int64_t counter = 0;
    for (const auto &[centroid, outer] : kcstars) {
        // Note: The verifier above could be simplified, since centroid and outer are already separated.
        auto kcstar = centroid.union_with(outer);

        if (!KCliqueStarVerifier(graph, kcstar, k)) {
            return false;
        }
        ++counter;
    }

    std::cout << "verified " << counter << " k-clique-stars" << std::endl;
    return true;
}

}