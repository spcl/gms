// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef BUILDER_H_
#define BUILDER_H_

#include <algorithm>
#include <cinttypes>
#include <fstream>
#include <functional>
#include <type_traits>
#include <utility>
#include <cstring>
#include <cassert>

#include "command_line.h"
#include "generator.h"
#include "graph.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "reader.h"
#include "timer.h"
#include "util.h"
#include "bitmap.h"

#include <gms/common/types.h>

#include <gms/representations/graphs/log_graph/kbit_adjacency_array.h>
#include <gms/representations/graphs/log_graph/kbit_adjacency_array_local.h>
#include <gms/representations/graphs/log_graph/kbit_weighted_adjacency_array.h>
#include <gms/representations/graphs/log_graph/kbit_weighted_adjacency_array_local.h>
#include <gms/representations/graphs/log_graph/bit_tree_graph.h>
#include <gms/representations/graphs/log_graph/my_bitmap.h>
#include <gms/representations/graphs/log_graph/options.h>
#include <gms/representations/graphs/coders/varint_byte_based_graph.h>
#include <gms/representations/graphs/coders/varint_word_based_graph.h>
#include <gms/representations/graphs/permuters/permuters.h>

// I'd like to put the following typedefs into 'benchmark.h',
// but if I do, there is a cyclic dependency with includes
#if LOCAL_APPROACH
	#if BIT_TREE
		typedef Bit_Tree_Graph My_Graph;
	#else
		typedef Kbit_Adjacency_Array_Local My_Graph;
	#endif
	typedef Kbit_Weighted_Adjacency_Array_Local My_Weighted_Graph;
#else
    #if COMPRESSED
        #if VARINT_BYTE_BASED
        typedef VarintByteBasedGraph My_Graph;
        #elif VARINT_WORD_BASED
        typedef VarintWordBasedGraph My_Graph;
        #endif
	#else
	typedef Kbit_Adjacency_Array My_Graph;
	#endif
	typedef Kbit_Weighted_Adjacency_Array My_Weighted_Graph;
#endif

// #define ALPHA 0.00390265
// #define ALPHA 0.015625
#define ALPHA 0.0078 // I now define ALPHA dynamically

#define REPORT_BITTREE_SIZE 1


/*
GAP Benchmark Suite
Class:  BuilderBase
Author: Scott Beamer

Given arguements from the command line (cli), returns a built graph
 - MakeGraph() will parse cli and obtain edgelist and call
   MakeGraphFromEL(edgelist) to perform actual graph construction
 - edgelist can be from file (reader) or synthetically generated (generator)
 - Common case: BuilderBase typedef'd (w/ params) to be Builder (benchmark.h)
*/

template <typename NodeId_, typename DestID_ = NodeId_,
		  typename WeightT_ = NodeId_, bool invert = true>
class BuilderBase {
  typedef EdgePair<NodeId_, DestID_> Edge;
  typedef pvector<Edge> EdgeList;

  const CLBase &cli_;
  bool symmetrize_;
  bool needs_weights_;
  int64_t num_nodes_ = -1;

  int64_t bits_saved_with_bittree_encoding = 0;
  int64_t bittree_encoding_space = 0;

 public:
  explicit BuilderBase(const CLBase &cli) : cli_(cli) {
	symmetrize_ = cli_.symmetrize();
	needs_weights_ = !std::is_same<NodeId_, DestID_>::value;
  }

  DestID_ GetSource(EdgePair<NodeId_, NodeId_> e) {
	return e.u;
  }

  DestID_ GetSource(EdgePair<NodeId_, NodeWeight<NodeId_, WeightT_>> e) {
	return NodeWeight<NodeId_, WeightT_>(e.u, e.v.w);
  }

  NodeId_ FindMaxNodeId(const EdgeList &el) {
	NodeId_ max_seen = 0;
	#pragma omp parallel for reduction(max : max_seen)
	for (auto it = el.begin(); it < el.end(); it++) {
	  Edge e = *it;
	  max_seen = std::max(max_seen, e.u);
	  max_seen = std::max(max_seen, (NodeId_) e.v);
	}
	return max_seen;
  }

  /* Return the largest weight in the CSR graph*/
  WeightT_ FindMaxWeight(const CSRGraphBase<NodeId_, DestID_, invert> &csr) {
	WeightT_ max_weight = 0;
	#pragma omp parallel for reduction(max : max_weight)
    for (NodeId u=0; u < csr.num_nodes(); u++) {
		for(DestID_ v : csr.out_neigh(u)){
	  		max_weight = std::max(max_weight, v.w);
		}
	}
	return max_weight;
  }

	NodeId_ FindMaxGap(const CSRGraphBase<NodeId_, DestID_, invert> &csr) {
		NodeId_ max_gap = 0;
		#pragma omp parallel for reduction(max : max_gap)
		for (NodeId_ u=0; u < csr.num_nodes(); u++) {
			NodeId current_vertex = 0;
			for(DestID_ v : csr.out_neigh(u)){
				max_gap = std::max(max_gap, v-current_vertex);
				current_vertex = v;
			}
		}
		return max_gap;
	}


  pvector<NodeId_> CountDegrees(const EdgeList &el, bool transpose) {
	pvector<NodeId_> degrees(num_nodes_, 0);
	#pragma omp parallel for
	for (auto it = el.begin(); it < el.end(); it++) {
	  Edge e = *it;
	  if (symmetrize_ || (!symmetrize_ && !transpose))
		fetch_and_add(degrees[e.u], 1);
	  if (symmetrize_ || (!symmetrize_ && transpose))
		fetch_and_add(degrees[(NodeId_) e.v], 1);
	}
	return degrees;
  }

  static
  pvector<SGOffset> PrefixSum(const pvector<NodeId_> &degrees) {
	pvector<SGOffset> sums(degrees.size() + 1);
	SGOffset total = 0;
	for (size_t n=0; n < degrees.size(); n++) {
	  sums[n] = total;
	  total += degrees[n];
	}
	sums[degrees.size()] = total;
	return sums;
  }

  static
  pvector<SGOffset> ParallelPrefixSum(const pvector<NodeId_> &degrees) {
	const size_t block_size = 1<<20;
	const size_t num_blocks = (degrees.size() + block_size - 1) / block_size;
	pvector<SGOffset> local_sums(num_blocks);
	#pragma omp parallel for
	for (size_t block=0; block < num_blocks; block++) {
	  SGOffset lsum = 0;
	  size_t block_end = std::min((block + 1) * block_size, degrees.size());
	  for (size_t i=block * block_size; i < block_end; i++)
		lsum += degrees[i];
	  local_sums[block] = lsum;
	}
	pvector<SGOffset> bulk_prefix(num_blocks+1);
	SGOffset total = 0;
	for (size_t block=0; block < num_blocks; block++) {
	  bulk_prefix[block] = total;
	  total += local_sums[block];
	}
	bulk_prefix[num_blocks] = total;
	pvector<SGOffset> prefix(degrees.size() + 1);
	#pragma omp parallel for
	for (size_t block=0; block < num_blocks; block++) {
	  SGOffset local_total = bulk_prefix[block];
	  size_t block_end = std::min((block + 1) * block_size, degrees.size());
	  for (size_t i=block * block_size; i < block_end; i++) {
		prefix[i] = local_total;
		local_total += degrees[i];
	  }
	}
	prefix[degrees.size()] = bulk_prefix[num_blocks];
	return prefix;
  }

  // Removes self-loops and redundant edges
  // Side effect: neighbor IDs will be sorted
  void SquishCSR(const CSRGraphBase<NodeId_, DestID_, invert> &g, bool transpose,
				 DestID_*** sq_index, DestID_** sq_neighs) {
	pvector<NodeId_> diffs(g.num_nodes());
	DestID_ *n_start, *n_end;
	#pragma omp parallel for private(n_start, n_end)
	for (NodeId_ n=0; n < g.num_nodes(); n++) {
	  if (transpose) {
		n_start = g.in_neigh(n).begin();
		n_end = g.in_neigh(n).end();
	  } else {
		n_start = g.out_neigh(n).begin();
		n_end = g.out_neigh(n).end();
	  }
	  std::sort(n_start, n_end);
	  DestID_ *new_end = std::unique(n_start, n_end);
	  new_end = std::remove(n_start, new_end, n);
	  diffs[n] = new_end - n_start;
	}
	pvector<SGOffset> sq_offsets = ParallelPrefixSum(diffs);
	*sq_neighs = new DestID_[sq_offsets[g.num_nodes()]];
	*sq_index = CSRGraphBase<NodeId_, DestID_>::GenIndex(sq_offsets, *sq_neighs);
	#pragma omp parallel for private(n_start)
	for (NodeId_ n=0; n < g.num_nodes(); n++) {
	  if (transpose)
		n_start = g.in_neigh(n).begin();
	  else
		n_start = g.out_neigh(n).begin();
	  std::copy(n_start, n_start+diffs[n], (*sq_index)[n]);
	}
  }

  CSRGraphBase<NodeId_, DestID_, invert> SquishGraph(
	  const CSRGraphBase<NodeId_, DestID_, invert> &g) {
	DestID_ **out_index, *out_neighs, **in_index, *in_neighs;
	SquishCSR(g, false, &out_index, &out_neighs);
	if (g.directed()) {
	  if (invert)
		SquishCSR(g, true, &in_index, &in_neighs);
	  return CSRGraphBase<NodeId_, DestID_, invert>(g.num_nodes(), out_index,
												out_neighs, in_index,
												in_neighs);
	} else {
	  return CSRGraphBase<NodeId_, DestID_, invert>(g.num_nodes(), out_index,
												out_neighs);
	}
  }

  /*
  Graph Bulding Steps (for CSR):
	- Read edgelist once to determine vertex degrees (CountDegrees)
	- Determine vertex offsets by a prefix sum (ParallelPrefixSum)
	- Allocate storage and set points according to offsets (GenIndex)
	- Copy edges into storage
  */
  void MakeCSR(const EdgeList &el, bool transpose, DestID_*** index,
			   DestID_** neighs) {
	pvector<NodeId_> degrees = CountDegrees(el, transpose);
	pvector<SGOffset> offsets = ParallelPrefixSum(degrees);
	cout << "creating offset array with " << offsets.size()*sizeof(SGOffset) << " bytes" << endl;
	cout << "creating array with " << offsets[num_nodes_]*sizeof(DestID_) << " bytes" << endl;
	*neighs = new DestID_[offsets[num_nodes_]];
	*index = CSRGraphBase<NodeId_, DestID_>::GenIndex(offsets, *neighs);
	#pragma omp parallel for
	for (auto it = el.begin(); it < el.end(); it++) {
	  Edge e = *it;
	  if (symmetrize_ || (!symmetrize_ && !transpose))
			(*neighs)[fetch_and_add(offsets[e.u], 1)] = e.v;
	  if (symmetrize_ || (!symmetrize_ && transpose))
		(*neighs)[fetch_and_add(offsets[static_cast<NodeId_>(e.v)], 1)] =
			GetSource(e);
	}
  }

  CSRGraphBase<NodeId_, DestID_, invert> MakeGraphFromEL(EdgeList &el) {
	DestID_ **index = nullptr, **inv_index = nullptr;
	DestID_ *neighs = nullptr, *inv_neighs = nullptr;
	Timer t;
	t.Start();
	if (num_nodes_ == -1)
	  num_nodes_ = FindMaxNodeId(el)+1;
	if (needs_weights_)
	  Generator<NodeId_, DestID_, WeightT_>::InsertWeights(el);
	MakeCSR(el, false, &index, &neighs);
	if (!symmetrize_ && invert)
	  MakeCSR(el, true, &inv_index, &inv_neighs);
	t.Stop();
	PrintTime("Build Time", t.Seconds());
	if (symmetrize_)
	  return CSRGraphBase<NodeId_, DestID_, invert>(num_nodes_, index, neighs);
	else
	  return CSRGraphBase<NodeId_, DestID_, invert>(num_nodes_, index, neighs,
												inv_index, inv_neighs);
  }

  template <class CGraph>
  CGraph MakeGraphFromELGeneric(EdgeList &el) {
      auto csr_graph = MakeGraphFromEL(el);
      SquishGraph(csr_graph);
      if constexpr (std::is_same_v<CGraph, CSRGraph>) {
          return csr_graph;
      } else {
          return csrToCGraphGeneric<CGraph>(csr_graph);
      }
  }

/* Allocates memory, given a number of entries and the desired bitlength.
   The number of bits allocated is at least 'entries'*'bitlength' large
   and is rounded up to the next number divisible by 64.
   This amount of unused memory is negligible but needed for a safe memory access.
   */
void* allocate_memory(int64_t entries, int32_t bitlength){
	// The calculation is made such as to avoid large numbers like m*k to
	// avoid overflowing 64 bits
	int64_t size;
	size = entries % 64 == 0 ?
	(entries/64)*bitlength :
	(entries/64)*bitlength + ((entries%64)*bitlength)/64 + 2;
	cout << "creating kbit adjacency array with " << size*sizeof(int64_t) << " bytes" << endl;
    return calloc(size, sizeof(int64_t));
}

/* Takes a CSRGraph and turns it into a Kbit_Adjacency_Array */
Kbit_Adjacency_Array csrToKbit(const CSRGraphBase<NodeId_, DestID_, invert> &csr){
	int64_t n = csr.num_nodes(); // number of vertices
	int64_t m = csr.num_edges(); // number of vertices
	bool directed = !symmetrize_;
	#if SIMPLE_GAP_ENCODING
		int8_t k = ceil(log2(FindMaxGap(csr)));
	#else
		int8_t k = ceil(log2(n)); // bitlength for vertex ID encoding
	#endif
	int64_t mask = ~((int64_t) -1 << k);

	// allocate structures
	cout << "creating offset array with " << (n+1)*sizeof(int64_t) << " bytes" << endl;
	int64_t* offsetArray = (int64_t*) calloc(n+1, sizeof(int64_t));
    int32_t* adjacencyArray = directed ?
		(int32_t*) allocate_memory(m, k) : (int32_t*) allocate_memory(2*m, k);
	for(int i=1; i<=n; i++){
		offsetArray[i] = offsetArray[i-1] + csr.out_degree(i-1);
	}

	for(NodeId u : csr.vertices()){
		int pos = 0;
		#if SIMPLE_GAP_ENCODING
			int64_t current_vertex = 0;
		#endif
		for(NodeId v : csr.out_neigh(u)){
			int64_t a, b, d, w;
			char* addr;
			int64_t to_store = v;
			a = k * (offsetArray[u] + pos++); // exact bit position where the first neighbour is stored
			b = a >> 3;
			d = a & 7;
			addr = (char*)adjacencyArray + b;
			w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
			#if SIMPLE_GAP_ENCODING
				to_store = to_store - current_vertex;
				current_vertex = v;
			#endif
			((int64_t*) addr)[0] = w | ((to_store & mask) << d);
		}
	}
	return Kbit_Adjacency_Array(n, k, m, directed, offsetArray, adjacencyArray);
}

/* Takes a CSRGraph and turns it into a Kbit_Adjacency_Array */
Kbit_Adjacency_Array_Local csrToKbitLocal(const CSRGraphBase<NodeId_, DestID_, invert> &csr){
	int64_t n = csr.num_nodes(); // number of vertices
	int64_t m = csr.num_edges(); // number of vertices
	bool directed = !symmetrize_;

	vector<NodeId> W = vector<NodeId>(n, 0); // bit-lengths for neighbourhoods
	#pragma omp parallel for
    for (NodeId u=0; u < csr.num_nodes(); u++) {
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
			for(DestID_ v : csr.out_neigh(u)){
				W[u] = max(W[u], v-current_vertex);
				current_vertex = v;
			}
		#else
			for(DestID_ v : csr.out_neigh(u)){
				W[u] = max(W[u], v);
			}
		#endif
	}
	#pragma omp parallel for
	for(unsigned int i=0; i<W.size(); i++){
		W[i] = W[i] == 0 ? 1 : ceil(log2(W[i]+1));
	}

	// allocate structures
	vector<int64_t> bit_offset(n+1, 0); // bit offset of neighbourhood in adjacency array
	cout << "creating offset array with " << (n+1)*sizeof(int64_t) << " bytes" << endl;
	for(int i=1; i<=n; i++){
		bit_offset[i] = bit_offset[i-1] + csr.out_degree(i-1)*W[i-1];
	}
    int32_t* adjacencyArray = (int32_t*) allocate_memory(bit_offset[n], 1);

	for(NodeId u : csr.vertices()){
		int pos = 0;
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
		#endif
		for(NodeId v : csr.out_neigh(u)){
			int64_t a, b, d, w;
			int64_t k = W[u];
			int64_t mask = ~((int64_t) -1 << k);
			char* addr;
			NodeId to_store = v;
			a = bit_offset[u] + k*(pos++); // exact bit position where theneighbour is stored
			b = a >> 3;
			d = a & 7;
			addr = (char*)adjacencyArray + b;
			w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
			#if SIMPLE_GAP_ENCODING
				to_store = v - current_vertex;
				current_vertex = v;
			#endif
			((int64_t*) addr)[0] = w | ((to_store & mask) << d);
		}
	}
	int64_t* O = (int64_t*) calloc(2*n, sizeof(int64_t));
	for(int i=0; i<n; i++){
		* (int32_t*) (O + 2*i) = (int32_t) csr.out_degree(i);
		*((int32_t*) (O + 2*i) + 1) = (int32_t)W[i];
		*(O + 2*i +1) = bit_offset[i];
	}
	return Kbit_Adjacency_Array_Local(n, m, directed,
		adjacencyArray, O);
}

/* Takes a CSRGraph and turns it into a Kbit_Weighted_Adjacency_Array */
Kbit_Weighted_Adjacency_Array csrToWeightedKbit(const CSRGraphBase<NodeId_, DestID_, invert> &csr){
	int64_t n = csr.num_nodes(); // number of vertices
	int64_t m = csr.num_edges(); // number of edges
	bool directed = !symmetrize_;
	#if SIMPLE_GAP_ENCODING
		int k = ceil(log2(FindMaxGap(csr)));
	#else
		int8_t k = ceil(log2(n)); // bitlength for vertex ID encoding
	#endif
	int8_t w = ceil(log2(FindMaxWeight(csr))); // bitlength for weight encoding
	int8_t kw = k+w; // bitlength for edge encoding
	int64_t mask = ~((int64_t) -1 << kw);

	// allocate structures
	cout << "creating offset array with " << (n+0)*sizeof(int64_t) << " bytes" << endl;
	int64_t* offsetArray = (int64_t*) calloc(n+1, sizeof(int64_t));
	for(int i=1; i<=n; i++){
		offsetArray[i] = offsetArray[i-1] + csr.out_degree(i-1);
	}
    int32_t* adjacencyArray = directed ?
		(int32_t*) allocate_memory(m, kw) : (int32_t*) allocate_memory(2*m, kw);

	// now we copy the edge information to log(Graph)
	for(NodeId u : csr.vertices()){
		int64_t pos = 0;
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
		#endif
		for(DestID_ v : csr.out_neigh(u)){
			int64_t a, b, d, orig;
			char* addr;
			a = kw * (offsetArray[u] + pos++); // exact bit position where the first neighbour is stored
			b = a >> 3;
			d = a & 7;
			int64_t to_store = ((int64_t)v.v << w) | v.w; // value to store
			addr = (char*)adjacencyArray + b;
			orig = ((int64_t*) addr)[0]; // 64-bit value currently stored here
			#if SIMPLE_GAP_ENCODING
				to_store = ((int64_t)(v.v - current_vertex) << w) | v.w; // value to store
				current_vertex = v.v;
			#endif
			((int64_t*) addr)[0] = orig | ((to_store & mask) << d);
		}
	}
	return Kbit_Weighted_Adjacency_Array(n, k, w, m, directed, offsetArray, adjacencyArray);
}

/* Takes a CSRGraph and turns it into a Kbit_Weighted_Adjacency_Array */
Kbit_Weighted_Adjacency_Array_Local csrToWeightedKbitLocal(const CSRGraphBase<NodeId_, DestID_, invert> &csr){
	int64_t n = csr.num_nodes(); // number of vertices
	int64_t m = csr.num_edges(); // number of edges
	bool directed = !symmetrize_;

	vector<NodeId> id_bitlength(n, 0); // per-neighbourhood bitlength for vertex IDs
	vector<NodeId> weight_bitlength(n, 0); // per-neighoburhood bitlength for weights
    for (NodeId u=0; u < csr.num_nodes(); u++) {
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
			for(DestID_ v : csr.out_neigh(u)){
				id_bitlength[u] = std::max(id_bitlength[u], v.v-current_vertex);
				weight_bitlength[u] = std::max(weight_bitlength[u], v.w);
				current_vertex = v;
			}
		#else
			for(DestID_ v : csr.out_neigh(u)){
				id_bitlength[u] = std::max(id_bitlength[u], v.v);
				weight_bitlength[u] = std::max(weight_bitlength[u], v.w);
			}
		#endif
	}
	#pragma omp parallel for
	for(unsigned int i=0; i<n; i++){
		id_bitlength[i] = id_bitlength[i] == 0 ? 1 : ceil(log2(id_bitlength[i]+1));
		weight_bitlength[i] = weight_bitlength[i] == 0 ? 1 : ceil(log2(weight_bitlength[i]+1));
	}

	// allocate structures
	cout << "creating offset array with " << (n+1)*sizeof(int64_t) << " bytes" << endl;
	vector<int64_t> bit_offset(n+1, 0); // bit offset of neighbourhood in adjacency array
	for(int i=1; i<=n; i++){
		bit_offset[i] = bit_offset[i-1] + csr.out_degree(i-1)*(id_bitlength[i-1]+weight_bitlength[i-1]);
	}
    int32_t* adjacencyArray = (int32_t*) allocate_memory(bit_offset[n], 1);

	for(NodeId u : csr.vertices()){
		int pos = 0;
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
		#endif
		for(DestID_ v : csr.out_neigh(u)){
			int64_t ebo, b, d, orig;
			int64_t kw = id_bitlength[u] + weight_bitlength[u];
			int64_t mask = ~((int64_t) -1 << kw);
			char* addr;
			int64_t to_store = ((int64_t)v.v << weight_bitlength[u]) | v.w; // value to store
			ebo = bit_offset[u] + kw*(pos++); // exact bit offset where theneighbour is stored
			b = ebo >> 3;
			d = ebo & 7;
			addr = (char*)adjacencyArray + b;
			orig = ((int64_t*) addr)[0]; // 64-bit value currently stored here
			#if SIMPLE_GAP_ENCODING
				to_store = ((int64_t)(v.v - current_vertex) << weight_bitlength[u]) | v.w; // value to store
				current_vertex = v;
			#endif
			((int64_t*) addr)[0] = orig | ((to_store & mask) << d);
		}
	}

	int64_t* O = (int64_t*) calloc(2*n, sizeof(int64_t));
	for(int i=0; i<n; i++){
		* (int32_t*) (O + 2*i) = (int32_t) csr.out_degree(i);
		*((int8_t*) (O + 2*i) + 4) = (int8_t)id_bitlength[i];
		*((int8_t*) (O + 2*i) + 5) = (int8_t)weight_bitlength[i];
		*(O + 2*i +1) = bit_offset[i];
	}
	return Kbit_Weighted_Adjacency_Array_Local(n, m, directed, adjacencyArray, O);
}



VarintByteBasedGraph csrToVarintByteBased(const CSRGraphBase<NodeId_, DestID_, invert> &csr) {

	bool directed = csr.directed();
	int64_t num_nodes = csr.num_nodes();
	int64_t num_edges = csr.num_edges();

	uint64_t* new_offsets_out = new uint64_t[csr.num_nodes()];
	uint64_t* new_offsets_in = new uint64_t[csr.num_nodes()];
    // Pessimistic estimate of required memory: at most 8 bytes per edge, and 9 per node (at most 8 bytes for varint
    // neighborhood degree, and 1 fixed byte for ascending/descending information).
	uint64_t new_adj_data_size = csr.num_edges() * 8 + csr.num_nodes() * 9;
	unsigned char* new_adj_data_out_tmp = new unsigned char[new_adj_data_size];
	unsigned char* new_adj_data_in_tmp = new unsigned char[new_adj_data_size];

	uint64_t itr_out = 0;
	uint64_t itr_in = 0;

	for (auto v : csr.vertices()) {
        assert(std::is_sorted(csr.out_neigh(v).begin(), csr.out_neigh(v).end()));

        new_offsets_out[v] = itr_out;
		new_offsets_in[v] = itr_in;
		uint64_t v_deg_out = csr.out_degree(v);
		uint64_t v_deg_in = csr.in_degree(v);
		int next_out = toVarint(v_deg_out, &new_adj_data_out_tmp[itr_out]);
		int next_in = toVarint(v_deg_in, &new_adj_data_in_tmp[itr_in]);
		itr_out += next_out;
		itr_in += next_in;

		NodeId prev_v = v;
		bool first_neigh = true;
		for (auto v2 : csr.out_neigh(v)) {
			uint64_t diff;
			if (first_neigh) {
				if (v2 < prev_v) {
					new_adj_data_out_tmp[itr_out] = NEXT_VAL_SMALLER;
					diff = prev_v - v2;
				} else {
					new_adj_data_out_tmp[itr_out] = NEXT_VAL_GREATER;
					diff = v2 - prev_v;
				}
				itr_out++;

				next_out = toVarint(diff, &new_adj_data_out_tmp[itr_out]);
				itr_out += next_out;
				first_neigh = false;
				prev_v = v2;
				continue;
			}

			diff = v2 - prev_v;
			next_out = toVarint(diff, &new_adj_data_out_tmp[itr_out]);
			itr_out += next_out;
			prev_v = v2;
		}

		prev_v = v;
		first_neigh = true;
		for (auto v2 : csr.in_neigh(v)) {
			uint64_t diff;
			if (first_neigh) {
				if (v2 < prev_v) {
					new_adj_data_in_tmp[itr_in] = NEXT_VAL_SMALLER;
					diff = prev_v - v2;
				} else {
					new_adj_data_in_tmp[itr_in] = NEXT_VAL_GREATER;
					diff = v2 - prev_v;
				}
				itr_in++;

				next_in = toVarint(diff, &new_adj_data_in_tmp[itr_in]);
				itr_in += next_in;
				first_neigh = false;
				prev_v = v2;
				continue;
			}

			diff = v2 - prev_v;
			next_in = toVarint(diff, &new_adj_data_in_tmp[itr_in]);
			itr_in += next_in;
			prev_v = v2;
		}
	}

	unsigned char* new_adj_data_out = new unsigned char[itr_out];
	unsigned char* new_adj_data_in = new unsigned char[itr_in];
	memcpy(&new_adj_data_out[0], &new_adj_data_out_tmp[0], itr_out);
	memcpy(&new_adj_data_in[0], &new_adj_data_in_tmp[0], itr_in);
	delete [] new_adj_data_out_tmp;
	delete [] new_adj_data_in_tmp;

	return VarintByteBasedGraph(num_nodes, num_edges, directed, new_offsets_out, new_offsets_in, new_adj_data_out, new_adj_data_in);
}



VarintWordBasedGraph csrToVarintWordBased(const CSRGraphBase<NodeId_, DestID_, invert> &csr) {

		bool directed = csr.directed();
		int64_t num_nodes = csr.num_nodes();
		int64_t num_edges = csr.num_edges();

		uint64_t* new_offsets_out = new uint64_t[csr.num_nodes()];
		uint64_t* new_offsets_in = new uint64_t[csr.num_nodes()];
		// Pessimistic estimate of required memory: at most 8 bytes per edge, and 9 per node but rounded to 16 to
		// account for word padding.
		uint64_t new_adj_data_size = csr.num_edges() * 8 + csr.num_nodes() * 16;
		unsigned char* new_adj_data_out_tmp = new unsigned char[new_adj_data_size];
		unsigned char* new_adj_data_in_tmp = new unsigned char[new_adj_data_size];

		uint64_t itr_out = 0;
		uint64_t itr_in = 0;

		for (auto v : csr.vertices()) {

			new_offsets_out[v] = itr_out >> 3;
			new_offsets_in[v] = itr_in >> 3;
			uint64_t v_deg_out = csr.out_degree(v);
			uint64_t v_deg_in = csr.in_degree(v);
			int next_out = toVarint(v_deg_out, &new_adj_data_out_tmp[itr_out]);
			int next_in = toVarint(v_deg_in, &new_adj_data_in_tmp[itr_in]);
			itr_out += next_out;
			itr_in += next_in;

			NodeId prev_v = v;
			bool first_neigh = true;
			for (auto v2 : csr.out_neigh(v)) {
				uint64_t diff;
				if (first_neigh) {
					if (v2 < prev_v) {
						new_adj_data_out_tmp[itr_out] = NEXT_VAL_SMALLER;
						diff = prev_v - v2;
					} else {
						new_adj_data_out_tmp[itr_out] = NEXT_VAL_GREATER;
						diff = v2 - prev_v;
					}
					itr_out++;

					next_out = toVarint(diff, &new_adj_data_out_tmp[itr_out]);
					itr_out += next_out;
					first_neigh = false;
					prev_v = v2;
					continue;
				}

				diff = v2 - prev_v;
				next_out = toVarint(diff, &new_adj_data_out_tmp[itr_out]);
				itr_out += next_out;
				prev_v = v2;
			}

			if(itr_out % BYTES_IN_64BIT_WORD != 0) {
				int add_bytes = ( BYTES_IN_64BIT_WORD - (itr_out % BYTES_IN_64BIT_WORD));
				itr_out += add_bytes;
			}

			prev_v = v;
			first_neigh = true;
			for (auto v2 : csr.in_neigh(v)) {
				uint64_t diff;
				if (first_neigh) {
					if (v2 < prev_v) {
						new_adj_data_in_tmp[itr_in] = NEXT_VAL_SMALLER;
						diff = prev_v - v2;
					} else {
						new_adj_data_in_tmp[itr_in] = NEXT_VAL_GREATER;
						diff = v2 - prev_v;
					}
					itr_in++;

					next_in = toVarint(diff, &new_adj_data_in_tmp[itr_in]);
					itr_in += next_in;
					first_neigh = false;
					prev_v = v2;
					continue;
				}

				diff = v2 - prev_v;
				next_in = toVarint(diff, &new_adj_data_in_tmp[itr_in]);
				itr_in += next_in;
				prev_v = v2;
			}

			if(itr_in % BYTES_IN_64BIT_WORD != 0) {
				int add_bytes = ( BYTES_IN_64BIT_WORD - (itr_in % BYTES_IN_64BIT_WORD));
				itr_in += add_bytes;
			}
		}

		unsigned char* new_adj_data_out = new unsigned char[itr_out];
		unsigned char* new_adj_data_in = new unsigned char[itr_in];
		std::memcpy(&new_adj_data_out[0], &new_adj_data_out_tmp[0], itr_out);
		std::memcpy(&new_adj_data_in[0], &new_adj_data_in_tmp[0], itr_in);
		delete [] new_adj_data_out_tmp;
		delete [] new_adj_data_in_tmp;

		return VarintWordBasedGraph(num_nodes, num_edges, directed, new_offsets_out, new_offsets_in, new_adj_data_out, new_adj_data_in);
	}

//Ignore Compiler-Warning of 3rd party Code (GMS team)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
	template <class CGraph>
	auto csrToCGraphGeneric(const CSRGraphBase<NodeId_, DestID_, invert> &csr_graph) {
        if constexpr (std::is_same_v<CGraph, CSRGraph>) {
            return csr_graph;
        } else if constexpr (std::is_same_v<CGraph, VarintByteBasedGraph>) {
            return csrToVarintByteBased(csr_graph);
        } else if constexpr (std::is_same_v<CGraph, VarintWordBasedGraph>) {
            return csrToVarintWordBased(csr_graph);
        } else if constexpr (std::is_same_v<CGraph, Kbit_Adjacency_Array>) {
            bool symm_backup = symmetrize_;
            symmetrize_ = !csr_graph.directed();
            auto g = csrToKbit(csr_graph);
            symmetrize_ = symm_backup;
            return std::move(g);
        } else if constexpr (std::is_same_v<CGraph, Kbit_Adjacency_Array_Local>) {
            bool symm_backup = symmetrize_;
            symmetrize_ = !csr_graph.directed();
            auto g = csrToKbitLocal(csr_graph);
            symmetrize_ = symm_backup;
            return std::move(g);
        } else {
            static_assert(GMS::always_false<CGraph>, "class not supported");
        }
    }
#pragma GCC diagnostic pop

struct Tree {
	Tree* left = 0; // left subtree
	Tree* right = 0; // right subtree
	Tree* parent = 0; // parent tree
	int32_t left_complete = 0; // the left subtree is complete

	Tree(){
	}

	Tree(Tree* parent_){
		parent = parent_;
	}
};

void remove_tree(Tree* current){
	if(current->left){
		remove_tree(current->left);
	}
	if(current->right){
		remove_tree(current->right);
	}
	delete current;
}

My_Bitmap* encode_as_bit_tree(
        const CSRGraphBase<NodeId_, DestID_, invert> &csr, NodeId u, int8_t k){

	if(csr.out_degree(u) == 0){
		return new My_Bitmap(0);
	}
	// cout << "neighbourhood of " << u << " is: " << endl;
	// for(int v : csr.in_neigh(u)){
	// 	cout << v << " " ;
	// }
	// cout << endl;

	// this is were we create and encode a bit tree
	// First we construct an actual tree with pointers
	Tree* root = new Tree();
	Tree* current = root;
	vector<Tree*> leafs = vector<Tree*>();
	// cout << "STEP 1: " << endl;
	int32_t size = 1; // this variable counts the number of nodes in the tree
	for(NodeId v : csr.out_neigh(u)){
		// cout << "looking at vertex " << v << endl;
		current = root;
		for(int i=0; i<k; i++){
			if((v >> (k-i-1)) & 1){
				if(!current->right){
					current->right = new Tree(current);
					// cout << "inserting right" << endl;
					size ++;
				}
				current = current->right;
			}
			else{
				if(!current->left){
					current->left = new Tree(current);
					// cout << "inserting left " << endl;
					size ++;
				}
				current = current->left;
			}
		}
		leafs.push_back(current);
	}
	// cout << "STEP 2: " << endl;
	// Optimization: We remove all complete subtrees
	for(Tree* leaf : leafs){
		// If 'leaf' is the left child of its parent, we mark that
		// the parent has a left subtree that is complete
		// If 'leaf' is the right child and the parent has a left
		// child, then we know that the parent is the root of a
		// complete subtree and we can apply the same algorithm
		// recursively on the parent as being the 'leaf'
		current = leaf;
		while(true){
			if(current->parent->left == current){
				current->parent->left_complete = true;
				break;
			}
			else if(current->parent->left_complete){
				current = current->parent;
				if(current->right != leaf){
					size -= 2;
				}
				// cout << "removing vertices" << endl;
				remove_tree(current->left);
				remove_tree(current->right);
				current->left = 0;
				current->right= 0;
			}
			else{
				break;
			}
		}
	}
	// cout << "STEP 3: " << endl;
	// Then we do a DFS and encode the tree
	vector<pair<Tree*, int> > stack = vector<pair<Tree*, int> >();
	My_Bitmap* encoding = new My_Bitmap(2*(size-leafs.size()));
	// encoding->reset(); // this was needed when using Beamer's Bitmap
	int32_t pos = 0;
	stack.push_back({root, 1});
	while(!stack.empty()){
		Tree* current = stack.back().first;
		int height = stack.back().second;
		stack.pop_back();
		if(current->right){
			// encoding->set_bit(pos+1);
			encoding->set_bit(pos); // because of endian
			if(height < k){
				stack.push_back({current->right, height + 1});
			}
		}
		if(current->left){
			// encoding->set_bit(pos);
			encoding->set_bit(pos +1); // because of endian
			if(height < k){
				stack.push_back({current->left, height + 1});
			}
		}
		pos += 2;
	}


	// #if REPORT_BITTREE_SIZE
		// cout << "bittree size: " << encoding->get_size() << "bits" << endl;
		// cout << "size if encoded with Log(Graph) : " << k*csr.out_degree(u) << "bits" << endl;
		// cout << "bit tree encoding saved : " << (k*csr.out_degree(u)) - encoding->get_size() << " bits" << endl;
		bits_saved_with_bittree_encoding += (k*csr.out_degree(u)) - encoding->get_size();
		if(k*csr.out_degree(u) - encoding->get_size() <= 0){
			cout << "LOOSING space while encoding neighbourhood " << u << "!!!!" << endl;
		}
		bittree_encoding_space += encoding->get_size();
	// #endif
	// cout << "done" << endl;
	// encoding->id = u;
	remove_tree(root);
	return encoding;
}

/* Takes a CSRGraph and turns it into a Kbit_Adjacency_Array */
Bit_Tree_Graph csrToBitTree(const CSRGraphBase<NodeId_, DestID_, invert> &csr){
	int64_t n = csr.num_nodes(); // number of vertices
	int64_t m = csr.num_edges(); // number of vertices
	bool directed = !symmetrize_;

	vector<NodeId> bitlength = vector<NodeId>(n, 0); // bit-lengths for neighbourhoods
	#if BIT_TREE
		vector<bool> bit_tree_encoding = vector<bool>(n, 0); // whether to use bit_tree encoding
		double alpha = 1.0/(pow(2.0,((log2(n)-2.0)/2.0))); // heuristic
		cout << "alpha: " << alpha << endl;
	#endif

	// #pragma omp parallel for
	// very strange race condition...
	// I do this sequentially for now
	int number_of_bittree_neighbourhoods = 0;
    for (NodeId u=0; u < csr.num_nodes(); u++) {
		// the following equation defines whether to use Bit Tree Encoding
		#if BIT_TREE
			// double alpha = alpha_heuristics[(int)ceil(log2(n))];
			// alpha = ALPHA;
			// double alpha = 0.0156;
			if((double)csr.out_degree(u) / (double)n >= alpha){
				bit_tree_encoding[u] = true;
				number_of_bittree_neighbourhoods ++;
				// cout << u << "!!!" <<  (double)csr.out_degree(u) / (double)n << endl;
			}
			// else{
				// cout << u << "!!!" <<  (double)csr.out_degree(u) / (double)n << endl;

			// }
		#endif
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
			for(DestID_ v : csr.out_neigh(u)){
				bitlength[u] = max(bitlength[u], v-current_vertex);
				current_vertex = v;
			}
		#else
			for(DestID_ v : csr.out_neigh(u)){
				bitlength[u] = max(bitlength[u], v);
			}
		#endif
	}
	#pragma omp parallel for
	for(unsigned int i=0; i<n; i++){
		bitlength[i] = bitlength[i] == 0 ? 1 : ceil(log2(bitlength[i]+1));
	}

	// allocate structures
	vector<int64_t> bit_offset(n+1, 0); // bit offset of neighbourhood in adjacency array
	for(int i=1; i<=n; i++){
		#if BIT_TREE
		if(bit_tree_encoding[i-1])
			bit_offset[i] = bit_offset[i-1];
		else
		#endif
			bit_offset[i] = bit_offset[i-1] + csr.out_degree(i-1)*bitlength[i-1];
	}
    int32_t* adjacencyArray = (int32_t*) allocate_memory(bit_offset[n], 1);
	Offset_Array_Entry* O = (Offset_Array_Entry*) calloc(n, sizeof(Offset_Array_Entry));
	cout << "creating offset array with " << (n)*sizeof(Offset_Array_Entry) << " bytes" << endl;

	for(NodeId u : csr.vertices()){
		int64_t k = bitlength[u];
		#if BIT_TREE
			if(bit_tree_encoding[u]){
				O[u].offset_or_tree.tree = encode_as_bit_tree(csr, u, k);
				continue;
			}
		#endif
		int pos = 0;
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
		#endif
		for(NodeId v : csr.out_neigh(u)){
			int64_t a, b, d, w;
			int64_t mask = ~((int64_t) -1 << k);
			char* addr;
			NodeId to_store = v;
			a = bit_offset[u] + k*(pos++); // exact bit position where theneighbour is stored
			b = a >> 3;
			d = a & 7;
			addr = (char*)adjacencyArray + b;
			w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
			#if SIMPLE_GAP_ENCODING
				to_store = v - current_vertex;
				current_vertex = v;
			#endif
			((int64_t*) addr)[0] = w | ((to_store & mask) << d);
		}
	}

	for(int i=0; i<n; i++){
		O[i].degree = (int32_t) csr.out_degree(i);
		O[i].bitlength = (int8_t) bitlength[i];
		#if BIT_TREE
			O[i].encoding = (int8_t) bit_tree_encoding[i];
			if (!bit_tree_encoding[i])
		#endif
		O[i].offset_or_tree.offset = bit_offset[i];
	}
	cout << "Bytes for bittree encoded neighourhoods: " << bittree_encoding_space/8 << endl;
	cout << "Bytes saved with bittree encoding: " << (bits_saved_with_bittree_encoding / 8) << endl;
	cout << "Percentage of bittree neighbourhoods: " << ((double)number_of_bittree_neighbourhoods/n) << endl;
	// cout << "done encoding the graph" << endl;
	return Bit_Tree_Graph(n, m, directed,
		adjacencyArray, O);
}



struct compareEdges {
  /* used to sort edges */
	bool operator () (Edge const& lhs, Edge const& rhs) {
		if (lhs.u == rhs.u) {
			return lhs.v < rhs.v;
	    }
	    else {
	    	return lhs.u < rhs.u;
	    }
	}
};


void remove_duplicates(EdgeList& el, pvector<NodeId_>& degrees, int64_t& m){
	// mark self loops with a high ID,
	// so they get sorted to the back of the list

	for(Edge &e : el){
		if(e.u == e.v){
			e.u = num_nodes_;
			e.v = num_nodes_;
		}
	}
	std::sort(el.begin(), el.end(), compareEdges());

	// shift the duplicate edges towards the end of the list
	unsigned int i = 0;
	unsigned int j = 1;
	m = 0; // in the following we also count the number of edges
	while(j < el.size()){
		// el[i] is a real edge, no duplicate and no self-loop
		if(el[i].u != el[j].u || el[i].v != el[j].v){
			degrees[el[i].u] ++;
			degrees[el[i].v] ++;
			i ++;
			el[i] = el[j];
			m ++;
		}
		j ++;
		if(el[j].u == num_nodes_){
			break;
		}
	}
	degrees[el[i].u] ++;
	degrees[el[i].v] ++;
	m ++;
	el.resize(m);
}

/* Version of 'MakeGraph()' that creates a Kbit_Adjacency_Array. */
 	Kbit_Adjacency_Array make_kbit_graph(){
		if (cli_.filename() != "") {
			return Kbit_Adjacency_Array(false);
		}
		else if (cli_.scale() != -1) {
			// Generate the specified graph. It comes back as a list of edges
			Generator<NodeId_, DestID_> gen(cli_.scale(), cli_.degree());
			EdgeList el = gen.GenerateEL(cli_.uniform());

			Timer t;
			t.Start();

			// Here we build the graph representation.
			// Because the graph generator generates multiple edges,
			// we need to remove those first.
			// We do this by sortng all edges and then pushing
			// the duplicate edges towards the end of the list.
			// If we are constructing an undirected graph, it is easier
			// if every undirected edge is represented only once in the edge list
			// So before removing duplicate edges we make sure that for every
			// edge 'e', 'e.u' is smaller than 'e.v'.
			if(symmetrize_){
				for(Edge &e : el){
					if(e.u > e.v){
						NodeId t = e.u;
						e.u = e.v;
						e.v = t;
					}
				}
			}
			if (num_nodes_ == -1)
				num_nodes_ = FindMaxNodeId(el)+1;
			pvector<NodeId_> degrees = pvector<NodeId_>(num_nodes_, 0);
			int64_t m = 0; // number of undirected edges
			remove_duplicates(el, degrees, m);
			int64_t n = num_nodes_; // number of vertices
			int32_t k = ceil(log2(num_nodes_));
		    int64_t mask = ~((int64_t) -1 << k);

			// allocate structures
			int64_t* offsetArray = (int64_t*) calloc(n+1, sizeof(int64_t));
			cout << "creating offset array with " << (n+1)*sizeof(int64_t) << " bytes" << endl;
			int64_t size_to_allocate;
			if(symmetrize_){
				size_to_allocate = m*k % 4 == 0 ? m*k/4 + 7 : m*k/4 + 8;
			}
			else{
				size_to_allocate = m*k % 8 == 0 ? m*k/8 + 7 : m*k/8 + 8;
			}
			cout << "creating kbit_array with " << size_to_allocate << " bytes" << endl;
		    int32_t* adjacencyArray = (int32_t*) malloc(size_to_allocate);
		    for(int i = 0; i < size_to_allocate/4; i++){
		        adjacencyArray[i] = 0;
		    }
			// offsets
			pvector<SGOffset> pos = ParallelPrefixSum(degrees);
			for(int u = 0; u <= num_nodes_; u++){
		    	offsetArray[u] = pos[u];
				pos[u] = 0;
			}

			// setting the edges
			for(Edge e : el){
				int64_t a, b, d, w;
				char* addr;
			    a = k * (offsetArray[e.u] + pos[e.u]++); // exact bit position where the first neighbour is stored
			    b = a >> 3;
			    d = a & 7;
			    addr = (char*)adjacencyArray + b;
			    w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
			    ((int64_t*) addr)[0] = w | ((e.v & mask) << d);
				if(symmetrize_){
					a = k * (offsetArray[e.v] + pos[e.v]++); // exact bit position where the first neighbour is stored
				    b = a >> 3;
				    d = a & 7;
				    addr = (char*)adjacencyArray + b;
				    w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
				    ((int64_t*) addr)[0] = w | ((e.u & mask) << d);
				}
			}

			t.Stop();
			PrintTime("Build Time", t.Seconds());
			if(symmetrize_){
				return Kbit_Adjacency_Array(n, k, m, false, offsetArray, adjacencyArray);
			}
			else{
				return Kbit_Adjacency_Array(n, k, m, true, offsetArray, adjacencyArray);
			}
		}
		else {
			return Kbit_Adjacency_Array(false);
		}
 	}


	std::map<NodeId, NodeId> get_reverse_permutation(std::map<NodeId, NodeId> permutation_map) {
		std::map<NodeId, NodeId> result_map;
		for (auto iter = permutation_map.begin(); iter != permutation_map.end(); iter++) {
			result_map[iter->second] = iter->first;
		}

		return result_map;
	}

	template <PermuterVariant TVariant>
    const CSRGraphBase<NodeId_, DestID_, invert> permute(CSRGraphBase<NodeId_, DestID_, invert> graph) {

        std::map<NodeId, NodeId> permutation;

        if constexpr (TVariant == PermuterVariant::OutDegreeAscending) {
            permutation = OutDegreeAscendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OutDegreeDescending) {
            permutation = OutDegreeDescendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::InDegreeAscending) {
            permutation = InDegreeAscendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::InDegreeDescending) {
            permutation = InDegreeDescendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        }
#ifdef CPLEX_ENABLED
        else if constexpr (TVariant == PermuterVariant::OptimalDiffNnIlpUnconstr) {
            permutation = OptimalDiffNNIlpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffNnLpUnconstr) {
            permutation = OptimalDiffNNLpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffNnIlpConstr) {
            permutation = OptimalDiffNNIlpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffNnLpConstr) {
            permutation = OptimalDiffNNLpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffVnIlpUnconstr) {
            permutation = OptimalDiffVNIlpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffVnLpUnconstr) {
            permutation = OptimalDiffVNLpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffVnIlpConstr) {
            permutation = OptimalDiffVNIlpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OptimalDiffVnLpConstr) {
            permutation = OptimalDiffVNLpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OIlpNnUnN) {
            permutation = OIlpNNUnNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OIlpNnConN) {
            permutation = OIlpNNConNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OIlpVnUnN) {
            permutation = OIlpVNUnNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        } else if constexpr (TVariant == PermuterVariant::OIlpVnConN) {
            permutation = OIlpVNConNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        }
#endif // CPLEX_ENABLED
        else {
            static_assert(GMS::always_false<TVariant>, "should be unreachable");
        }

        DestID_** new_out_index = new DestID_*[graph.num_nodes() + 1]();
        DestID_* new_out_neighbours = new DestID_[graph.num_edges() * (graph.directed() ? 1 : 2)];

        std::map<NodeId, NodeId> reverse_permutation;
        reverse_permutation = get_reverse_permutation(permutation);


        uint64_t offset_index = 0;
        uint64_t neigh_index = 0;
        for (auto v : graph.vertices()) {
            NodeId v_old = reverse_permutation[v];
            new_out_index[offset_index] = (new_out_neighbours + neigh_index);

            for (auto u_old : graph.out_neigh(v_old)) {
                new_out_neighbours[neigh_index] = permutation[u_old];
                neigh_index++;
            }

            std::sort(new_out_index[offset_index], new_out_neighbours + neigh_index);
            offset_index++;
        }
        new_out_index[graph.num_nodes()] = new_out_neighbours + (graph.num_edges() * (graph.directed() ? 1 : 2));


        if (graph.directed()) {
            DestID_** new_in_index = new DestID_*[graph.num_nodes() + 1]();
            DestID_* new_in_neighbours = new DestID_[graph.num_edges() * (graph.directed() ? 1 : 2)];

            offset_index = 0;
            neigh_index = 0;
            for (auto v : graph.vertices()) {
                NodeId v_old = reverse_permutation[v];
                new_in_index[offset_index] = (new_in_neighbours + neigh_index);

                for (auto u_old : graph.in_neigh(v_old)) {
                    new_in_neighbours[neigh_index] = permutation[u_old];
                    neigh_index++;
                }

                std::sort(new_in_index[offset_index], new_in_neighbours + neigh_index);
                offset_index++;
            }
            new_in_index[graph.num_nodes()] = new_in_neighbours + (graph.num_edges() * (graph.directed() ? 1 : 2));

            return CSRGraphBase<NodeId_, DestID_, invert>(graph.num_nodes(), new_out_index, new_out_neighbours, new_in_index, new_in_neighbours);
        } else {

            return CSRGraphBase<NodeId_, DestID_, invert>(graph.num_nodes(), new_out_index, new_out_neighbours);
        }
    }

    // TODO deprecate or forward to the template permute function
	const CSRGraphBase<NodeId_, DestID_, invert> permute(CSRGraphBase<NodeId_, DestID_, invert> graph) {

		std::map<NodeId, NodeId> permutation;

		#if OUT_ASCENDING
			permutation = OutDegreeAscendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        #elif OUT_DESCENDING
			permutation = OutDegreeDescendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        #elif IN_ASCENDING
			permutation = InDegreeAscendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
        #elif IN_DESCENDING
			permutation = InDegreeDescendingPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
		#elif CPLEX_ENABLED
			#if OPTIMAL_DIFF_NN_ILP_UNCONSTR
				permutation = OptimalDiffNNIlpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_NN_LP_UNCONSTR
				permutation = OptimalDiffNNLpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_NN_ILP_CONSTR
				permutation = OptimalDiffNNIlpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_NN_LP_CONSTR
				permutation = OptimalDiffNNLpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_VN_ILP_UNCONSTR
				permutation = OptimalDiffVNIlpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_VN_LP_UNCONSTR
				permutation = OptimalDiffVNLpUnconstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_VN_ILP_CONSTR
				permutation = OptimalDiffVNIlpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif OPTIMAL_DIFF_VN_LP_CONSTR
				permutation = OptimalDiffVNLpConstrPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif O_ILP_NN_UN_N
				permutation = OIlpNNUnNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif O_ILP_NN_CON_N
				permutation = OIlpNNConNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif O_ILP_VN_UN_N
				permutation = OIlpVNUnNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#elif O_ILP_VN_CON_N
				permutation = OIlpVNConNPermuter<NodeId_, DestID_, invert>::permutation_map(graph);
			#endif
		#endif

		DestID_** new_out_index = new DestID_*[graph.num_nodes() + 1]();
		DestID_* new_out_neighbours = new DestID_[graph.num_edges() * (graph.directed() ? 1 : 2)];

		std::map<NodeId, NodeId> reverse_permutation;
		reverse_permutation = get_reverse_permutation(permutation);


		uint64_t offset_index = 0;
		uint64_t neigh_index = 0;
		for (auto v : graph.vertices()) {
			NodeId v_old = reverse_permutation[v];
			new_out_index[offset_index] = (new_out_neighbours + neigh_index);

			for (auto u_old : graph.out_neigh(v_old)) {
                new_out_neighbours[neigh_index] = permutation[u_old];
				neigh_index++;
			}

			std::sort(new_out_index[offset_index], new_out_neighbours + neigh_index);
			offset_index++;
		}
		new_out_index[graph.num_nodes()] = new_out_neighbours + (graph.num_edges() * (graph.directed() ? 1 : 2));


		if (graph.directed()) {
			DestID_** new_in_index = new DestID_*[graph.num_nodes() + 1]();
			DestID_* new_in_neighbours = new DestID_[graph.num_edges() * (graph.directed() ? 1 : 2)];

			offset_index = 0;
			neigh_index = 0;
			for (auto v : graph.vertices()) {
				NodeId v_old = reverse_permutation[v];
				new_in_index[offset_index] = (new_in_neighbours + neigh_index);

				for (auto u_old : graph.in_neigh(v_old)) {
                    new_in_neighbours[neigh_index] = permutation[u_old];
					neigh_index++;
				}

				std::sort(new_in_index[offset_index], new_in_neighbours + neigh_index);
				offset_index++;
			}
			new_in_index[graph.num_nodes()] = new_in_neighbours + (graph.num_edges() * (graph.directed() ? 1 : 2));

			return CSRGraphBase<NodeId_, DestID_, invert>(graph.num_nodes(), new_out_index, new_out_neighbours, new_in_index, new_in_neighbours);
		} else {

			return CSRGraphBase<NodeId_, DestID_, invert>(graph.num_nodes(), new_out_index, new_out_neighbours);
		}
	}



	My_Graph make_graph_from_CSR(){
		#if PERMUTED
			return make_graph_from_CSR(permute(MakeGraph()));
    	#else
			return make_graph_from_CSR(MakeGraph());
        #endif
	}

	My_Graph make_graph_from_CSR(const CSRGraphBase<NodeId_, DestID_, invert>& csr){
		#if LOCAL_APPROACH
			#if BIT_TREE
			return csrToBitTree(csr);
			#else
			return csrToKbitLocal(csr);
			#endif
        #else
            #if COMPRESSED
                #if VARINT_BYTE_BASED
					return csrToVarintByteBased(csr);
                #elif VARINT_WORD_BASED
					return csrToVarintWordBased(csr);
                #endif
            #else
			return csrToKbit(csr);
			#endif
		#endif
	}

	My_Weighted_Graph make_weighted_graph_from_CSR(){
		CSRGraphBase<NodeId_, DestID_, invert> csr = MakeGraph();
		#if LOCAL_APPROACH
			return csrToWeightedKbitLocal(csr);
		#else
			return csrToWeightedKbit(csr);
		#endif
	}

	My_Graph make_graph(){
		#if LOCAL_APPROACH
			return make_kbit_local_graph();
		#else
			return make_kbit_graph();
		#endif
	}

/* Version of 'MakeGraph()' that creates a Kbit_Adjacency_Array. */
 	Kbit_Adjacency_Array_Local make_kbit_local_graph(){
		if (cli_.filename() != "") {
			return Kbit_Adjacency_Array_Local(false);
		}
		if (cli_.scale() == -1) {
			return Kbit_Adjacency_Array_Local(false);
		}
		// Generate the specified graph. It comes back as a list of edges
		Generator<NodeId_, DestID_> gen(cli_.scale(), cli_.degree());
		EdgeList el = gen.GenerateEL(cli_.uniform());

		Timer t;
		t.Start();

		// Here we build the graph representation.
		// Because the graph generator generates multiple edges,
		// we need to remove those first.
		// We do this by sortng all edges and then pushing
		// the duplicate edges towards the end of the list.
		// If we are constructing an undirected graph, it is easier
		// if every undirected edge is represented only once in the edge list
		// So before removing duplicate edges we make sure that for every
		// edge 'e', 'e.u' is smaller than 'e.v'.
		for(Edge &e : el){
			if(e.u > e.v){
				NodeId t = e.u;
				e.u = e.v;
				e.v = t;
			}
		}
		if (num_nodes_ == -1)
			num_nodes_ = FindMaxNodeId(el)+1;
		pvector<NodeId_> degrees = pvector<NodeId_>(num_nodes_, 0);
		int64_t m = 0; // number of edges
		remove_duplicates(el, degrees, m);
		int64_t n = num_nodes_; // number of vertices


		vector<NodeId> W = vector<NodeId>(n, 0); // bit-length for neighbourhoods
		for(Edge e : el){
			W[e.u] = max(W[e.u], e.v);
			W[e.v] = max(W[e.v], e.u);
		}
		for(int& Wi : W){
			if(Wi == 0){
				Wi = 1;
			}
			else{
				Wi = ceil(log2(Wi+1));
			}
		}

		// allocate structures
		// offset array: The first 59 bits contain the exact bit offset in
		// the adjacency array where the i'th neighbourhood is stored
		// The last 5 bits store the bit lenght with wich the neighbours
		// of the i'th vertex are encoded.
	    // int64_t mask_5 = ~((int64_t) -1 << 5); // selects first 5 bits
	    int64_t mask_59 = ~((int64_t) -1 << 59); // selectes first 59 bits
		int64_t* offsetArray = (int64_t*) calloc(n+1, sizeof(int64_t));
		cout << "creating offset array with " << (n+1)*sizeof(int64_t) << " bytes" << endl;
		for(int i=1; i<=n; i++){
			offsetArray[i] = offsetArray[i-1] + degrees[i-1]*W[i-1];
			offsetArray[i-1] = offsetArray[i-1] | ((int64_t)W[i-1] << 59);
		}

		// adjacency array:
		int64_t size_to_allocate = offsetArray[n] % 8 == 0 ?
				offsetArray[n]/8 + 7 : offsetArray[n]/8 + 8;
		cout << "creating kbit_array with " << size_to_allocate << " bytes" << endl;
	    int32_t* adjacencyArray = (int32_t*) malloc(size_to_allocate);
	    for(int i = 0; i < size_to_allocate/4; i++){
	        adjacencyArray[i] = 0;
	    }
		// setting the edges
		vector<int32_t> pos = vector<int32_t> (n, 0);
		for(Edge e : el){
			int64_t a, b, d, w;
			char* addr;
			int64_t k = W[e.u];
	    	int64_t mask = ~((int64_t) -1 << k);
			a = (offsetArray[e.u]&mask_59) + k*(pos[e.u]++); // exact bit position where the first neighbour is stored
		    b = a >> 3;
		    d = a & 7;
		    addr = (char*)adjacencyArray + b;
		    w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
		    ((int64_t*) addr)[0] = w | ((e.v & mask) << d);
			k = W[e.v];
			a = (offsetArray[e.v]&mask_59) + k*(pos[e.v]++); // exact bit position where the first neighbour is stored
		    b = a >> 3;
		    d = a & 7;
		    addr = (char*)adjacencyArray + b;
		    w = ((int64_t*) addr)[0]; // 64-bit value currently stored here
		    ((int64_t*) addr)[0] = w | ((e.u & mask) << d);
		}

		t.Stop();
		PrintTime("Build Time", t.Seconds());
		return Kbit_Adjacency_Array_Local(n, m, false, adjacencyArray, offsetArray);
 	}

	void build_tree_neighbourhood(vector<NodeId_>& neighbours){
		int k = ceil(log2(num_nodes_));
		pvector<bool> tree = pvector<bool> (2*num_nodes_);
		// create a full tree in which a node is set to true if it exists
		for(NodeId_ u : neighbours){
			int pos = 0;
			for(int i=1; i<k; i++){
				pos = pos*2 + 1;
				if((u>>(k-i-1)) & 1){
					pos += 1;
				}
			}
			cout <<pos<<endl;
		}
		// create the ecoded version of the tree
		const int LEFT = 2;
		const int RIGHT = 1;
		const int BOTH = 3;
		const int NONE = 0;
		//char* space = (char*)malloc(2*num_nodes_);
		//int64_t bit_offset = 0;
		vector<pair<int, int>> stack = vector<pair<int, int> >();
		//stack.push_back(1);
		while(!stack.empty()){
			int pos = 0; //stack.back();
			stack.pop_back();
			//int64_t byte_offset = bit_offset / BYTE;
			int code;
			if(tree[pos] == 0){
				code = NONE;
			}
			else{
				if(tree[2*pos+1]){
					code = LEFT;
				}
				if(tree[2*pos+1]){
					code = (code == LEFT) ? BOTH : RIGHT;
				}
			}
		}
	}


/* Version of 'MakeGraph()' that creates a Bit_Tree_Graph. */
//	Bit_Tree_Graph make_bit_tree_graph(){
//		if (cli_.scale() != -1) {
//			// Generate the specified graph. It comes back as a list of edges
//			Generator<NodeId_, DestID_> gen(cli_.scale(), cli_.degree());
//			EdgeList el = gen.GenerateEL(cli_.uniform());
//
//			Timer t;
//			t.Start();
//
//			if (num_nodes_ == -1){
//				num_nodes_ = FindMaxNodeId(el)+1;
//			}
//
//			pvector<NodeId_> degrees = pvector<NodeId_>(num_nodes_, 0);
//			int64_t m = 0; // number of edges
//			remove_duplicates(el, degrees, m);
//			vector<vector<NodeId_> > adjacency_vectors = vector<vector<NodeId_> >(num_nodes_,
//				vector<NodeId_>());
//			for(Edge e: el){
//				adjacency_vectors[e.u].push_back(e.v);
//				adjacency_vectors[e.v].push_back(e.u);
//			}
//			for(vector<NodeId_> neighbourhood : adjacency_vectors){
//				build_tree_neighbourhood(neighbourhood);
//			}
//			t.Stop();
//			PrintTime("Build Time", t.Seconds());
//
//			int32_t k = cli_.scale();
//
//			// First we create intermediate structures that are easier to create
//			// Here every vertex gets a bit tree of size 2n
//			// vector<vector<char> > intermediate_trees = vector<vector<char> >();
//			vector<vector<char> > intermediate_trees =
//				vector<vector<char> >(num_nodes_, vector<char> (2*k*k, false));
//			// for(int i = 0; i < num_nodes_; i++){
//			// 	intermediate_trees.push_back(new;
//			// }
//
//			// Now we fill it with the edge data:
//			// for(Edge e : el){
//			// 	// cout << "foo" << endl;
//			// 	// cout << "k is " << k << endl;
//			// 	// cout << "inserting " << e.v << endl;
//			// 	if(e.v > num_nodes_){
//			// 		cout << "!!!!!!!!!!!!!!l" << endl;
//			// 	}
//			// 	int pos = 0;
//			// 	for(int i = k-1; i >= 0; i--){
//			// 		int v = (e.v >> i) & 1;
//			// 		// cout << "v is " << v << endl;
//			// 		pos = 2*pos + 1 + v;
//			// 		// cout << "pos is " << pos << endl;
//			// 		intermediate_trees[e.u][pos] = true;
//			// 	}
//			// }
//			return Bit_Tree_Graph(0, 0, 0, false);
//		}
//		else {
//			return Bit_Tree_Graph(0, 0, 0, false);
//		}
//	}

  CSRGraphBase<NodeId_, DestID_, invert> MakeGraph() {
	CSRGraphBase<NodeId_, DestID_, invert> g;
	{  // extra scope to trigger earlier deletion of el (save memory)
	  EdgeList el;
	  if (cli_.filename() != "") {
		Reader<NodeId_, DestID_, WeightT_, invert> r(cli_.filename());
		if ((r.GetSuffix() == ".sg") || (r.GetSuffix() == ".wsg")) {
		  return r.ReadSerializedGraph();
		} else {
		  el = r.ReadFile(needs_weights_);
		}
	  } else if (cli_.scale() != -1) {
		Generator<NodeId_, DestID_> gen(cli_.scale(), cli_.degree());
		el = gen.GenerateEL(cli_.uniform());
	  }
	  g = MakeGraphFromEL(el);
	}
	return SquishGraph(g);
  }

    CSRGraphBase<NodeId_, DestID_, invert> MakeGraph(std::string filename, bool DAGify = false)
    {
        CSRGraphBase<NodeId_, DestID_, invert> g;
        {  // extra scope to trigger earlier deletion of el (save memory)
            EdgeList el;
            if (filename != "") {
                Reader<NodeId_, DestID_, WeightT_, invert> r(filename);
                if ((r.GetSuffix() == ".sg") || (r.GetSuffix() == ".wsg")) {
                    return r.ReadSerializedGraph();
                } else {
                    el = r.ReadFile(needs_weights_);
                }
            } else if (cli_.scale() != -1) {
                Generator<NodeId_, DestID_> gen(cli_.scale(), cli_.degree());
                el = gen.GenerateEL(cli_.uniform());
            }
            //-----------------added by Greg--------------//
            if (DAGify) {
                symmetrize_ = false;
                EdgeList elDag;
                for (auto edge : el) {
                    if (edge.u < edge.v) {
                        elDag.push_back(edge);
                    }
                }
                g = MakeGraphFromEL(elDag);
            }
                //---------------end added by Greg------------//
            else {
                g = MakeGraphFromEL(el);
            }
        }
        return SquishGraph(g);
    }

  // Relabels (and rebuilds) graph by order of decreasing degree
  static
  CSRGraphBase<NodeId_, DestID_, invert> RelabelByDegree(
	  const CSRGraphBase<NodeId_, DestID_, invert> &g) {
	if (g.directed()) {
	  std::cout << "Cannot relabel directed graph" << std::endl;
	  std::exit(-11);
	}
	Timer t;
	t.Start();
	typedef std::pair<int64_t, NodeId_> degree_node_p;
	pvector<degree_node_p> degree_id_pairs(g.num_nodes());
	#pragma omp parallel for
	for (NodeId_ n=0; n < g.num_nodes(); n++)
	  degree_id_pairs[n] = std::make_pair(g.out_degree(n), n);
	std::sort(degree_id_pairs.begin(), degree_id_pairs.end(),
			  std::greater<degree_node_p>());
	pvector<NodeId_> degrees(g.num_nodes());
	pvector<NodeId_> new_ids(g.num_nodes());
	#pragma omp parallel for
	for (NodeId_ n=0; n < g.num_nodes(); n++) {
	  degrees[n] = degree_id_pairs[n].first;
	  new_ids[degree_id_pairs[n].second] = n;
	}
	pvector<SGOffset> offsets = ParallelPrefixSum(degrees);
	DestID_* neighs = new DestID_[offsets[g.num_nodes()]];
	DestID_** index = CSRGraphBase<NodeId_, DestID_>::GenIndex(offsets, neighs);
	#pragma omp parallel for
	for (NodeId_ u=0; u < g.num_nodes(); u++) {
	  for (NodeId_ v : g.out_neigh(u))
		neighs[offsets[new_ids[u]]++] = new_ids[v];
	  std::sort(index[new_ids[u]], index[new_ids[u]+1]);
	}
	t.Stop();
	PrintTime("Relabel", t.Seconds());
	return CSRGraphBase<NodeId_, DestID_, invert>(g.num_nodes(), index, neighs);
  }
};

#endif  // BUILDER_H_
