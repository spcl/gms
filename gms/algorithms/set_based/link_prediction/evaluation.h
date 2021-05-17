#pragma once

#include <random>
#include <algorithm>

#include <gms/representations/sets/roaring_set.h>
#include <gms/algorithms/set_based/vertex_similarity/vertex_similarity.h>
#include "undirected_edge.h"
#include "link_prediction.h"
#include "edge_sampler.h"
#include <gms/third_party/fast_statistics.h>
#include <omp.h>

/// Implementation for evaluation of link prediction.

namespace GMS::LinkPrediction {

template <class SGraph>
void add_undirected_edge(SGraph &g, NodeId u, NodeId v)
{
    g.out_neigh(u).add(v);
    g.out_neigh(v).add(u);
}

template <class SGraph>
void remove_undirected_edge(SGraph &g, NodeId u, NodeId v)
{
    g.out_neigh(u).remove(v);
    g.out_neigh(v).remove(u);
}

template <class SGraph>
void extract_random_test_edges(SGraph &g_train, SGraph &g_test, int64_t test_edges_required) {
    assert(count_undirected_edges(g_test) == 0);
    assert(count_undirected_edges(g_train) >= test_edges_required);

    EdgeSampler sampler(g_train);
    FastStatistics::WyRandRng rng(0);

    auto remove_test_edges = [&](){
        // Remove the test edges from the train graph.
        // Note that we do this in two steps for now since otherwise EdgeSampler would be invalidated.
        int64_t num_nodes = g_train.num_nodes();
        for (NodeId u = 0; u < num_nodes; ++u) {
            for (NodeId v : g_test.out_neigh(u)) {
                remove_undirected_edge(g_train, u, v);
            }
        }
    };

    int64_t num_edges = 0;
#ifndef NDEBUG
    int accepted = 0;
#endif
    int rejections = 0;
    while (num_edges < test_edges_required) {
        UndirectedEdge edge = sampler.sample(rng);
        if (!g_test.out_neigh(edge.first).contains(edge.second)) {
            add_undirected_edge(g_test, edge.first, edge.second);
            ++num_edges;
#ifndef NDEBUG
            ++accepted;
#endif
        } else {
            ++rejections;
        }
        if (rejections > 100) {
            remove_test_edges();
            sampler.rebuild();
            rejections = 0;
        }
        /*
#ifndef NDEBUG
        if (num_edges % 32 == 0) {
            std::cout << "sampled " << num_edges << "/" << test_edges_required << std::endl;
            std::cout << " acc " << accepted << ", rej " << rejections << "\n";
        }
#endif
        */
    }

    remove_test_edges();
}

struct LinkPredictionScore {
    double precision;
    double recall;
};

/**
 * Compute precision and recall scores for a number of predicted links given the true link information.
 *
 * @tparam EdgeSet implements Set interface, but type is an UndirectedEdge (i.e. usually SortedSetBase<UndirectedEdge>)
 * @tparam SGraph
 * @param e_predicted the predicted edges
 * @param g_true the true link graph
 * @return
 */
template <class EdgeSet, class SGraph>
LinkPredictionScore score_link_prediction_precision(const EdgeSet &e_predicted, const SGraph &g_true) {
    int64_t true_positives = 0;
    int64_t true_count = 0;

#pragma omp parallel for reduction(+: true_positives, true_count)
    for (NodeId u = 0; u < g_true.num_nodes(); ++u) {
        for (NodeId v : g_true.out_neigh(u)) {
            if (u < v) {
                UndirectedEdge edge(u, v);
                if (e_predicted.contains(edge)) {
                    ++true_positives;
                }
                ++true_count;
            }
        }
    }

    LinkPredictionScore score;
    // Precision = TP / (TP + FP)
    score.precision = double(true_positives) / double(e_predicted.cardinality());
    // Recall = TP / P
    score.recall = double(true_positives) / double(true_count);

    return score;
}

/**
 * Compute AUC scores by sampling true and false edges, and computing similarity values on the fly.
 *
 * @tparam SimilarityMeasure
 * @tparam SGraph
 * @param g_true the true input graph, containing all edges
 * @param g_train the input graph whose information is allowed to be used for training
 * @param g_test the input graph on which the predictions are evaluated
 * @param num_trials the fixed number of samples to be taken, currently there is no way to adaptively tune this
 * @return
 */
template <VertexSim::Metric SimilarityMeasure, class SGraph>
double score_link_prediction_auc(const SGraph &g_true, const SGraph &g_train, const SGraph &g_test, const int64_t num_trials)
{
    double higher_score = 0;
    double equal_score = 0;

    EdgeSampler true_sampler(g_true, false, true);
    EdgeSampler test_sampler(g_test, true, false);

    #pragma omp parallel
    {
        FastStatistics::WyRandRng rng(omp_get_thread_num());

        #pragma omp for reduction(+:higher_score) reduction(+:equal_score)
        for (int64_t i = 0; i < num_trials; ++i) {
            // Sample a true and a false edge.
            UndirectedEdge true_edge = test_sampler.sample(rng);
            UndirectedEdge false_edge;
            do {
                false_edge = true_sampler.sample_complement(rng);
                // Repeat until we find a complement edge of g_test which isn't an actual true edge which was removed
                // for train/test split.
            } while (g_test.out_neigh(false_edge.first).contains(false_edge.second));

            // Score the edges.
            double score_t = VertexSim::vertex_similarity<SimilarityMeasure>(true_edge.first, true_edge.second, g_train);
            double score_f = VertexSim::vertex_similarity<SimilarityMeasure>(false_edge.first, false_edge.second, g_train);

            if (score_t > score_f) {
                higher_score += 1.0;
            } else if (score_t == score_f) {
                equal_score += 1.0;
            }
        }
    }

    return (higher_score + 0.5 * equal_score) / double(num_trials);
}

/**
 * Randomly add some false links to a graph by replacing true links by false links.
 *
 * @tparam SGraph
 * @param train_graph output graph
 * @param mutations the number of mutations to be made
 * @param test_graph edges contained in this are actually true edges, thus they are skipped while sampling
 */
template <class SGraph>
void add_false_links(SGraph &train_graph, int64_t mutations, const SGraph &test_graph) {
    FastStatistics::WyRandRng rng(42);
    EdgeSampler sampler(train_graph);
    for (int64_t i = 0; i < mutations; ++i) {
        UndirectedEdge edge_remove = sampler.sample(rng);
        UndirectedEdge edge_create;
        do {
            edge_create = sampler.sample_complement(rng);
        } while (test_graph.out_neigh(edge_create.first).contains(edge_create.second));

        // perform the mutation
        remove_undirected_edge(train_graph, edge_remove.first, edge_remove.second);
        add_undirected_edge(train_graph, edge_create.first, edge_create.second);
        sampler.rebuild();
    }
}
}