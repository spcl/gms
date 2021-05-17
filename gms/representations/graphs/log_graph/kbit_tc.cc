// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <vector>

#include <gms/third_party/gapbs/benchmark.h>
#include <gms/third_party/gapbs/builder.h>
#include <gms/third_party/gapbs/command_line.h>
#include "kbit_adjacency_array.h"
#include <gms/third_party/gapbs/pvector.h>


/*
GAP Benchmark Suite
Kernel: Triangle Counting (TC)
Author: Scott Beamer

Will count the number of triangles (cliques of size 3)

Requires input graph:
  - to be undirected
  - no duplicate edges (or else will be counted as multiple triangles)
  - neighborhoods are sorted by vertex identifiers

Other than symmetrizing, the rest of the requirements are done by SquishCSR
during graph building.

This implementation reduces the search space by counting each triangle only
once. A naive implementation will count the same triangle six times because
each of the three vertices (u, v, w) will count it in both ways. To count
a triangle only once, this implementation only counts a triangle if u > v > w.
Once the remaining unexamined neighbors identifiers get too big, it can break
out of the loop, but this requires that the neighbors to be sorted.

Another optimization this implementation has is to relabel the vertices by
degree. This is beneficial if the average degree is high enough and if the
degree distribution is sufficiently non-uniform. To decide whether or not
to relabel the graph, we use the heuristic in WorthRelabelling.
*/


using namespace std;

size_t OrderedCount(const My_Graph &g) {
	size_t total = 0;
	#pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
	for (NodeId u=0; u < g.num_nodes(); u++) {
		// for (NodeId v : g.out_neigh(u)) {
		ITERATE_NEIGHBOURHOOD(v, u,
			if (v > u){
				break;
			}
			// auto it = g.out_neigh(u).begin();
			ITERATE_NEIGHBOURHOOD_2(it, u,
			// for (NodeId w : g.out_neigh(v)) {
			ITERATE_NEIGHBOURHOOD(w, v,
				if (w > v){
					break;
				}
				// This is an optimization that is needed because iterators
				// here are not just memory addresses
				// In the previous implementation '*it' was dereferenced twice,
				// once in the while loop and once afterwards.
				// No we derefrence it only once and store it in 'x'
				NodeId x = *it;
				while (x < w){
					x = *(++it);
				}
				if (w == x){
					total++;
				}
			)
			)
		)
	}
	return total;
}

//size_t myCount(const My_Graph &g){
//    size_t total = 0;
//
//  #pragma omp parallel for reduction(+ : total) schedule(dynamic, 64)
//    for (NodeId u=0; u < g.num_nodes(); u++) {
//        // for (NodeId w1 : g.out_neigh(u)) {
//		ITERATE_NEIGHBOURHOOD(w1, u,
//            // for (NodeId w2 : g.out_neigh(u)) {
//			ITERATE_NEIGHBOURHOOD(w2, u,
//                if(g.connected(w1, w2)){
//                    total ++;
//                }
//            )
//        )
//    }
//    return total / 6;
//}


// uses heuristic to see if worth relabeling
// size_t Hybrid(const Graph &g) {
//   if (WorthRelabelling(g))
//     return OrderedCount(Builder::KbitRelabelByDegree(g));
//   else
//     return OrderedCount(g);
// }


// uses heuristic to see if worth relabeling
My_Graph relabelIfNecessary(const CSRGraph &g, Builder b){
    if (WorthRelabelling(g)){
        return b.make_graph_from_CSR(Builder::RelabelByDegree(g));
    }
    else {
        return b.make_graph_from_CSR(g);
    }
}

void PrintTriangleStats(const My_Graph &g, size_t total_triangles) {
  cout << total_triangles << " triangles" << endl;
}

// Compares with simple serial implementation that uses std::set_intersection
bool TCVerifier(const My_Graph &g, size_t test_total) {
  size_t total = 0;
  vector<NodeId> intersection;
  intersection.reserve(g.num_nodes());
  for (NodeId u : g.vertices()) {
    // for (NodeId v : g.out_neigh(u)) {
	ITERATE_NEIGHBOURHOOD(v, u,
      #if BIT_TREE
		if(g.encoding(u) and g.encoding(v)) {
	      auto new_end = set_intersection(g.bit_tree_neigh(u).begin(),
	                                      g.bit_tree_neigh(u).end(),
	                                      g.bit_tree_neigh(v).begin(),
	                                      g.bit_tree_neigh(v).end(),
	                                      intersection.begin());
      	  intersection.resize(new_end - intersection.begin());
		}
		else if(g.encoding(u)) {
	      auto new_end = set_intersection(g.bit_tree_neigh(u).begin(),
	                                      g.bit_tree_neigh(u).end(),
	                                      g.out_neigh(v).begin(),
	                                      g.out_neigh(v).end(),
	                                      intersection.begin());
      	  intersection.resize(new_end - intersection.begin());
		}
		else if(g.encoding(v)) {
	      auto new_end = set_intersection(g.out_neigh(u).begin(),
	                                      g.out_neigh(u).end(),
	                                      g.bit_tree_neigh(v).begin(),
	                                      g.bit_tree_neigh(v).end(),
	                                      intersection.begin());
      	  intersection.resize(new_end - intersection.begin());
		}
		else{
	      auto new_end = set_intersection(g.out_neigh(u).begin(),
	                                      g.out_neigh(u).end(),
	                                      g.out_neigh(v).begin(),
	                                      g.out_neigh(v).end(),
	                                      intersection.begin());
      	  intersection.resize(new_end - intersection.begin());
		}
	  #else
      auto new_end = set_intersection(g.out_neigh(u).begin(),
                                      g.out_neigh(u).end(),
                                      g.out_neigh(v).begin(),
                                      g.out_neigh(v).end(),
                                      intersection.begin());
      intersection.resize(new_end - intersection.begin());
	  #endif
      total += intersection.size();
    )
  }
  total = total / 6;  // each triangle was counted 6 times
  if (total != test_total)
    cout << total << " != " << test_total << endl;
  return total == test_total;
}

int main(int argc, char* argv[]) {
    CLApp cli(argc, argv, "triangle count");
    if (!cli.ParseArgs())
        return -1;
    Builder b(cli);
    // Kbit_Adjacency_Array g = relabelIfNecessary(b.MakeGraph(), b);
    My_Graph g = relabelIfNecessary(b.MakeGraph(), b);
    if (g.directed()) {
        cout << "Input graph is directed but tc requires undirected" << endl;
        return -2;
    }
    BenchmarkKernelLegacy(cli, g, OrderedCount, PrintTriangleStats, TCVerifier);
    return 0;
}
