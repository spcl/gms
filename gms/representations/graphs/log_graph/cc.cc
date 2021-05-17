// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <gms/third_party/gapbs/benchmark.h>
#include <gms/third_party/gapbs/bitmap.h>
#include <gms/third_party/gapbs/builder.h>
#include <gms/third_party/gapbs/command_line.h>
#include <gms/third_party/gapbs/graph.h>
#include <gms/third_party/gapbs/pvector.h>
#include <gms/third_party/gapbs/timer.h>


/*
GAP Benchmark Suite
Kernel: Connected Components (CC)
Author: Scott Beamer

Will return comp array labelling each vertex with a connected component ID

This CC implementation makes use of the Shiloach-Vishkin [2] algorithm with
implementation optimizations from Bader et al. [1].

[1] David A Bader, Guojing Cong, and John Feo. "On the architectural
    requirements for efficient execution of graph algorithms." International
    Conference on Parallel Processing, Jul 2005.

[2] Yossi Shiloach and Uzi Vishkin. "An o(logn) parallel connectivity algorithm"
    Journal of Algorithms, 3(1):57–67, 1982.
*/


using namespace std;

pvector<NodeId> ShiloachVishkin(const CSRGraph &g) {
  pvector<NodeId> comp(g.num_nodes());
  #pragma omp parallel for
  for (NodeId n=0; n < g.num_nodes(); n++)
    comp[n] = n;
  bool change = true;
  int num_iter = 0;
  while (change) {
    change = false;
    num_iter++;
    #pragma omp parallel for
    for (NodeId u=0; u < g.num_nodes(); u++) {
      NodeId comp_u = comp[u];
      for (NodeId v : g.out_neigh(u)) {
        NodeId comp_v = comp[v];
        if ((comp_u < comp_v) && (comp_v == comp[comp_v])) {
          change = true;
          comp[comp_v] = comp_u;
        }
      }
    }
    #pragma omp parallel for
    for (NodeId n=0; n < g.num_nodes(); n++) {
      while (comp[n] != comp[comp[n]]) {
        comp[n] = comp[comp[n]];
      }
    }
  }
  #if PRINT_INFO
  	cout << "Shiloach-Vishkin took " << num_iter << " iterations" << endl;
  #endif
  return comp;
}


void PrintCompStats(const CSRGraph &g, const pvector<NodeId> &comp) {
  cout << endl;
  unordered_map<NodeId, NodeId> count;
  for (NodeId comp_i : comp)
    count[comp_i] += 1;
  int k = 5;
  vector<pair<NodeId, NodeId>> count_vector;
  count_vector.reserve(count.size());
  for (auto kvp : count)
    count_vector.push_back(kvp);
  vector<pair<NodeId, NodeId>> top_k = TopK(count_vector, k);
  k = min(k, static_cast<int>(top_k.size()));
  cout << k << " biggest clusters" << endl;
  for (auto kvp : top_k)
    cout << kvp.second << ":" << kvp.first << endl;
  cout << "There are " << count.size() << " components" << endl;
}


// Verifies CC result by performing a BFS from a vertex in each component
// - Asserts search does not reach a vertex with a different component label
// - If the graph is directed, it performs the search as if it was undirected
// - Asserts every vertex is visited (degree-0 vertex should have own label)
bool CCVerifier(const CSRGraph &g, const pvector<NodeId> &comp) {
  unordered_map<NodeId, NodeId> label_to_source;
  for (NodeId n : g.vertices())
    label_to_source[comp[n]] = n;
  Bitmap visited(g.num_nodes());
  visited.reset();
  vector<NodeId> frontier;
  frontier.reserve(g.num_nodes());
  for (auto label_source_pair : label_to_source) {
    NodeId curr_label = label_source_pair.first;
    NodeId source = label_source_pair.second;
    frontier.clear();
    frontier.push_back(source);
    visited.set_bit(source);
    for (auto it = frontier.begin(); it != frontier.end(); it++) {
      NodeId u = *it;
      for (NodeId v : g.out_neigh(u)) {
        if (comp[v] != curr_label)
          return false;
        if (!visited.get_bit(v)) {
          visited.set_bit(v);
          frontier.push_back(v);
        }
      }
      if (g.directed()) {
        for (NodeId v : g.in_neigh(u)) {
          if (comp[v] != curr_label)
            return false;
          if (!visited.get_bit(v)) {
            visited.set_bit(v);
            frontier.push_back(v);
          }
        }
      }
    }
  }
  for (NodeId n=0; n < g.num_nodes(); n++)
    if (!visited.get_bit(n))
      return false;
  return true;
}


int main(int argc, char* argv[]) {
  CLApp cli(argc, argv, "connected-components");
  if (!cli.ParseArgs())
    return -1;
  Builder b(cli);
  CSRGraph g = b.MakeGraph();
  BenchmarkKernelLegacy(cli, g, ShiloachVishkin, PrintCompStats, CCVerifier);
  return 0;
}
