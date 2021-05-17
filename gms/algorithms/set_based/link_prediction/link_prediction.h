#pragma once

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <vector>
#include <random>

#include <gms/common/types.h>
#include <gms/algorithms/set_based/vertex_similarity/vertex_similarity.h>
#include <gms/representations/graphs/set_graph.h>

#include <gms/common/format.h>

#include "undirected_edge.h"

namespace GMS::LinkPrediction {

/**
 * Output of link prediction.
 *
 * The edges are the best q edges and scores are the respective scores for each edge.
 */
struct ScoredEdges {
    std::vector<UndirectedEdge> edges;
    std::vector<double> scores;
};

/**
 * Implementation of vertex similarity-based Link prediction as described in [1].
 *
 * [1] D. Liben‐Nowell and J. Kleinberg, “The link-prediction problem for social networks,”
 *     Journal of the American Society for Information Science and Technology,
 *     vol. 58, no. 7, pp. 1019–1031, 2007, doi: 10.1002/asi.20591.
 *
 * @tparam SimilarityMeasure
 * @tparam SGraph
 * @param graph
 * @param q_best_edges
 * @return
 */
template <VertexSim::Metric SimilarityMeasure, class SGraph>
ScoredEdges link_prediction_similarity(
        const SGraph &graph,
        int64_t q_best_edges) {

    // Holds similarity score for q best edges, in ascending order,
    // i.e. last score[q-1] will be the best and score[0] will be the q-best.
    //
    // They have to be initialized to a value smaller than zero, e.g. -1.0 since scores can also be 0.
    std::vector<double> best_scores(q_best_edges, -1.0);
    // The q best edges in G's
    std::vector<UndirectedEdge> best_edges(q_best_edges);

    // Iterate over all non-existent edges in the graph.
    int64_t num_nodes = graph.num_nodes();
    for (NodeId u = 0; u < num_nodes; ++u) {
        const auto &neigh = graph.out_neigh(u);
        for (NodeId v = u + 1; v < num_nodes; ++v) {
            if (!neigh.contains(v)) {
                // Compute the similarity score of the vertices along the edge.
                double cur_score = VertexSim::vertex_similarity<SimilarityMeasure>(u, v, graph);

                // And now determine the best q edges.
                int64_t cur_rank = 0;
                while (cur_score > best_scores[cur_rank]) {
                    cur_rank++;
                    if (cur_rank >= q_best_edges)
                        break;
                }
                for (int64_t i = 0; i < cur_rank - 1; i++){
                    best_scores[i] = best_scores[i + 1];
                    best_edges[i] = best_edges[i + 1];
                }
                if (cur_rank > 0) {
                    best_scores[cur_rank - 1] = cur_score;
                    best_edges[cur_rank - 1] = UndirectedEdge(std::min(u, v), std::max(u, v));
                }
            }
        }
    }

    // Resize to remove unused values.
    if (best_scores[q_best_edges - 1] == -1.0) {
        int64_t limit = q_best_edges - 1;
        while (limit > 0 && best_scores[limit - 1] == -1.0) {
            --limit;
        }
        best_edges.resize(limit + 1);
        best_scores.resize(limit + 1);
    }

    // Return results
    ScoredEdges se;
    se.edges = best_edges;
    se.scores = best_scores;

    //gms::printArray("scores", best_scores.data(), best_scores.size());

    return se;
}
}