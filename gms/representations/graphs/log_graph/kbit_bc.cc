// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <functional>
#include <iostream>
#include <vector>

#include <gms/third_party/gapbs/benchmark.h>
#include <gms/third_party/gapbs/bitmap.h>
#include <gms/third_party/gapbs/builder.h>
#include <gms/third_party/gapbs/command_line.h>
#include <gms/third_party/gapbs/graph.h>
#include <gms/third_party/gapbs/platform_atomics.h>
#include <gms/third_party/gapbs/pvector.h>
#include <gms/third_party/gapbs/sliding_queue.h>
#include <gms/third_party/gapbs/timer.h>
#include <gms/third_party/gapbs/util.h>


/*
GAP Benchmark Suite
Kernel: Betweenness Centrality (BC)
Author: Scott Beamer

Will return array of approx betweenness centrality scores for each vertex

This BC implementation makes use of the Brandes [1] algorithm with
implementation optimizations from Madduri et al. [2]. It is only an approximate
because it does not compute the paths from every start vertex, but only a small
subset of them. Additionally, the scores are normalized to the range [0,1].

As an optimization to save memory, this implementation uses a Bitmap to hold
succ (list of successors) found during the BFS phase that are used in the back-
propagation phase.

[1] Ulrik Brandes. "A faster algorithm for betweenness centrality." Journal of
    Mathematical Sociology, 25(2):163–177, 2001.

[2] Kamesh Madduri, David Ediger, Karl Jiang, David A Bader, and Daniel
    Chavarria-Miranda. "A faster parallel algorithm and efficient multithreaded
    implementations for evaluating betweenness centrality on massive datasets."
    International Symposium on Parallel & Distributed Processing (IPDPS), 2009.
*/


using namespace std;
typedef float ScoreT;


int calc_edge_position(const My_Graph &g, NodeId source_node) {
    int result = 0;
    for (NodeId v : g.vertices()) {
        if (v == source_node) break;
        result += g.out_degree(v);
    }

    return result;
}

void PBFS(const My_Graph &g, NodeId source, pvector<NodeId> &path_counts,
    Bitmap &succ, vector<SlidingQueue<NodeId>::iterator> &depth_index,
    SlidingQueue<NodeId> &queue
	) {
  pvector<NodeId> depths(g.num_nodes(), -1);
  depths[source] = 0;
  path_counts[source] = 1;
  queue.push_back(source);
  depth_index.push_back(queue.begin());
  queue.slide_window();
  #pragma omp parallel
  {
    int depth = 0;
    QueueBuffer<NodeId> lqueue(queue);
    while (!queue.empty()) {
      #pragma omp single
      depth_index.push_back(queue.begin());
      depth++;
      #pragma omp for schedule(dynamic, 64)
      for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) {
        NodeId u = *q_iter;
        #ifdef COMPRESSED
            int64_t offset = calc_edge_position(g, u);
        #else
            int64_t offset = g.getOffset(u);
        #endif
        // for (NodeId v : g.out_neigh(u)) {
	 	ITERATE_NEIGHBOURHOOD(v, u,
          if ((depths[v] == -1) && (compare_and_swap(depths[v], -1, depth))) {
            lqueue.push_back(v);
          }
          if (depths[v] == depth) {
            // this is equal to the position of v in the adjacency array
            // succ.set_bit_atomic(v - g_out_start);
            succ.set_bit_atomic(offset);
            // this works !!!
            fetch_and_add(path_counts[v], path_counts[u]);
          }
          offset ++;
        )
      }
      lqueue.flush();
      #pragma omp barrier
      #pragma omp single
      queue.slide_window();
    }
  }
  depth_index.push_back(queue.begin());
}


pvector<ScoreT> Brandes(const My_Graph &g, SourcePicker<My_Graph> &sp,
                        NodeId num_iters) {
	#if PRINT_INFO
		Timer t;
		t.Start();
	#endif
	pvector<ScoreT> scores(g.num_nodes(), 0);
	pvector<NodeId> path_counts(g.num_nodes());
	Bitmap succ(g.num_edges_directed());
	vector<SlidingQueue<NodeId>::iterator> depth_index;
	SlidingQueue<NodeId> queue(g.num_nodes());
	#if PRINT_INFO
		t.Stop();
		PrintStep("a", t.Seconds());
	#endif
  for (NodeId iter=0; iter < num_iters; iter++) {
    NodeId source = sp.PickNext();
	#if PRINT_INFO
    	cout << "source: " << source << endl;
    	t.Start();
	#endif
    path_counts.fill(0);
    depth_index.resize(0);
    queue.reset();
    succ.reset();
    PBFS(g, source, path_counts, succ, depth_index, queue);
	#if PRINT_INFO
	    t.Stop();
	    PrintStep("b", t.Seconds());
	#endif
    pvector<ScoreT> deltas(g.num_nodes(), 0);
	#if PRINT_INFO
    	t.Start();
	#endif
    for (int d=depth_index.size()-2; d >= 0; d--) {
      #pragma omp parallel for schedule(dynamic, 64)
      for (auto it = depth_index[d]; it < depth_index[d+1]; it++) {
        NodeId u = *it;
        ScoreT delta_u = 0;
        #ifdef COMPRESSED
          int64_t offset = calc_edge_position(g, u);
        #else
          int64_t offset = g.getOffset(u);
        #endif
        // for (NodeId v : g.out_neigh(u)) {
	 	ITERATE_NEIGHBOURHOOD(v, u,
          if (succ.get_bit(offset)) {
            delta_u += static_cast<ScoreT>(path_counts[u]) /
                       static_cast<ScoreT>(path_counts[v]) * (1 + deltas[v]);
          }
          offset ++;
        )
        deltas[u] = delta_u;
        scores[u] += delta_u;
      }
    }
	#if PRINT_INFO
	    t.Stop();
	    PrintStep("p", t.Seconds());
	#endif
  }
  // normalize scores
  ScoreT biggest_score = 0;
  #pragma omp parallel for reduction(max : biggest_score)
  for (NodeId n=0; n < g.num_nodes(); n++)
    biggest_score = max(biggest_score, scores[n]);
  #pragma omp parallel for
  for (NodeId n=0; n < g.num_nodes(); n++)
    scores[n] = scores[n] / biggest_score;
  return scores;
}


void PrintTopScores(const My_Graph &g, const pvector<ScoreT> &scores) {
  vector<pair<NodeId, ScoreT>> score_pairs(g.num_nodes());
  for (NodeId n : g.vertices())
    score_pairs[n] = make_pair(n, scores[n]);
  int k = 5;
  vector<pair<ScoreT, NodeId>> top_k = TopK(score_pairs, k);
  for (auto kvp : top_k)
    cout << kvp.second << ":" << kvp.first << endl;
}


// Still uses Brandes algorithm, but has the following differences:
// - serial (no need for atomics or dynamic scheduling)
// - uses vector for BFS queue
// - regenerates farthest to closest traversal order from depths
// - regenerates successors from depths
bool BCVerifier(const My_Graph &g, SourcePicker<My_Graph> &sp, NodeId num_iters,
                const pvector<ScoreT> &scores_to_test) {
  pvector<ScoreT> scores(g.num_nodes(), 0);
  for (int iter=0; iter < num_iters; iter++) {
    NodeId source = sp.PickNext();
    // BFS phase, only records depth & path_counts
    pvector<int> depths(g.num_nodes(), -1);
    depths[source] = 0;
    vector<NodeId> path_counts(g.num_nodes(), 0);
    path_counts[source] = 1;
    vector<NodeId> to_visit;
    to_visit.reserve(g.num_nodes());
    to_visit.push_back(source);
    for (auto it = to_visit.begin(); it != to_visit.end(); it++) {
      NodeId u = *it;
    //   for (NodeId v : g.out_neigh(u)) {
	  ITERATE_NEIGHBOURHOOD(v, u,
        if (depths[v] == -1) {
          depths[v] = depths[u] + 1;
          to_visit.push_back(v);
        }
        if (depths[v] == depths[u] + 1)
          path_counts[v] += path_counts[u];
      )
    }
    // Get lists of vertices at each depth
    vector<vector<NodeId>> verts_at_depth;
    for (NodeId n : g.vertices()) {
      if (depths[n] != -1) {
        if (depths[n] >= static_cast<int>(verts_at_depth.size()))
          verts_at_depth.resize(depths[n] + 1);
        verts_at_depth[depths[n]].push_back(n);
      }
    }
    // Going from farthest to clostest, compute "depencies" (deltas)
    pvector<ScoreT> deltas(g.num_nodes(), 0);
    for (int depth=verts_at_depth.size()-1; depth >= 0; depth--) {
      for (NodeId u : verts_at_depth[depth]) {
        // for (NodeId v : g.out_neigh(u)) {
	 	ITERATE_NEIGHBOURHOOD(v, u,
          if (depths[v] == depths[u] + 1) {
            deltas[u] += static_cast<ScoreT>(path_counts[u]) /
                         static_cast<ScoreT>(path_counts[v]) * (1 + deltas[v]);
          }
        )
        scores[u] += deltas[u];
      }
    }
  }
  // Normalize scores
  ScoreT biggest_score = *max_element(scores.begin(), scores.end());
  for (NodeId n : g.vertices())
    scores[n] = scores[n] / biggest_score;
  // Compare scores
  bool all_ok = true;
  for (NodeId n : g.vertices()) {
    if (scores[n] != scores_to_test[n]) {
      cout << n << ": " << scores[n] << " != " << scores_to_test[n] << endl;
      all_ok = false;
    }
  }
  return all_ok;
}


int main(int argc, char* argv[]) {
  CLIterApp cli(argc, argv, "betweenness-centrality", 1);
  if (!cli.ParseArgs())
    return -1;
  if (cli.num_iters() > 1 && cli.start_vertex() != -1)
    cout << "Warning: iterating from same source (-r & -i)" << endl;
  Builder b(cli);

  // My_Graph g = b.make_graph();
  My_Graph g = b.make_graph_from_CSR();

  #if LOCAL_APPROACH
    g.createOffsetArray();
  #endif
  SourcePicker<My_Graph> sp(g, cli.start_vertex());
  auto BCBound =
    [&sp, &cli] (const My_Graph &g) { return Brandes(g, sp, cli.num_iters()); };
  SourcePicker<My_Graph> vsp(g, cli.start_vertex());
  auto VerifierBound = [&vsp, &cli] (const My_Graph &g,
                                     const pvector<ScoreT> &scores) {
    return BCVerifier(g, vsp, cli.num_iters(), scores);
  };
  BenchmarkKernelLegacy(cli, g, BCBound, PrintTopScores, VerifierBound);
  return 0;
}
