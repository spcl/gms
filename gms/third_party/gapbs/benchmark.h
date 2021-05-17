// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <algorithm>
#include <cinttypes>
#include <functional>
#include <random>
#include <utility>
#include <vector>
#include <tuple>

#include "builder.h"
#include "graph.h"
#include "timer.h"
#include "util.h"
#include "writer.h"

#include <gms/common/types.h>

/*
GAP Benchmark Suite
File:   Benchmark
Author: Scott Beamer

Various helper functions to ease writing of kernels
*/

#ifndef PRINT_DEBUG
#define PRINT_DEBUG 0
#endif

// Default type signatures for commonly used types
// Note: This typedef has been replaced by NodeId
//typedef int32_t NodeId;
typedef int32_t WeightT;
typedef NodeWeight<NodeId, WeightT> WNode;

typedef CSRGraphBase<NodeId, WNode> WGraph;

typedef BuilderBase<NodeId, NodeId, WeightT> Builder;
typedef BuilderBase<NodeId, WNode, WeightT> WeightedBuilder;

typedef WriterBase<NodeId, NodeId> Writer;
typedef WriterBase<NodeId, WNode> WeightedWriter;

// Used to pick random non-zero degree starting points for search algorithms
template <typename GraphT_>
class SourcePicker
{
public:
  explicit SourcePicker(const GraphT_ &g, NodeId given_source = -1)
      : given_source(given_source), rng(kRandSeed), udist(0, g.num_nodes() - 1),
        g_(g) {}

  NodeId PickNext()
  {
    if (given_source != -1)
      return given_source;
    NodeId source;
    do
    {
      source = udist(rng);
    } while (g_.out_degree(source) == 0);
    return source;
  }

private:
  NodeId given_source;
  std::mt19937 rng;
  std::uniform_int_distribution<NodeId> udist;
  const GraphT_ &g_;
};

// Returns k pairs with largest values from list of key-value pairs
template <typename KeyT, typename ValT>
std::vector<std::pair<ValT, KeyT>> TopK(
    const std::vector<std::pair<KeyT, ValT>> &to_sort, size_t k)
{
  std::vector<std::pair<ValT, KeyT>> top_k;
  ValT min_so_far = 0;
  for (auto kvp : to_sort)
  {
    if ((top_k.size() < k) || (kvp.second > min_so_far))
    {
      top_k.push_back(std::make_pair(kvp.second, kvp.first));
      std::sort(top_k.begin(), top_k.end(),
                std::greater<std::pair<ValT, KeyT>>());
      if (top_k.size() > k)
        top_k.resize(k);
      min_so_far = top_k.back().first;
    }
  }
  return top_k;
}

bool VerifyUnimplemented(...)
{
  std::cout << "** verify unimplemented **" << std::endl;
  return false;
}

// Calls (and times) kernel according to command line arguments
//
// This function shouldn't be used for new code.
template<typename GraphT_, typename GraphFunc, typename AnalysisFunc,
        typename VerifierFunc>
void BenchmarkKernelLegacy(const CLApp &cli, const GraphT_ &g,
                     GraphFunc kernel, AnalysisFunc stats,
                     VerifierFunc verify) {
    g.PrintStats();
    //cout << cli.GetName() << endl;
    double total_seconds = 0;
    Timer trial_timer;
    vector<double> times = vector<double>();
    for (int iter=0; iter < cli.num_trials(); iter++) {
        trial_timer.Start();
        auto result = kernel(g);
        trial_timer.Stop();
#if PRINT_LABELS
        PrintTime("Trial Time", trial_timer.Seconds());
#else
        printf("%3.5lf\n", trial_timer.Seconds());
#endif
        times.push_back(trial_timer.Seconds());
        total_seconds += trial_timer.Seconds();
        if (cli.do_analysis() && (iter == (cli.num_trials()-1))){
            stats(g, result);
        }
        if (cli.do_verify()) {
#if PRINT_LABELS
            trial_timer.Start();
			PrintLabel("Verification",
			verify(std::ref(g), std::ref(result)) ? "PASS" : "FAIL");
	    	trial_timer.Stop();
	    	PrintTime("Verification Time", trial_timer.Seconds());
#else
            if(!verify(std::ref(g), std::ref(result))){
                PrintLabel("VERIFICATION FAILED!", "");
                PrintLabel("aborting...", "");
                break;
            }
#endif
        }
    }
    // Here we also print the median, which is the n/2 best measurement.
    if(cli.num_trials() > 0){
        sort(times.begin(), times.end());
        PrintTime("Median Time", times[times.size()/2]);
    }
    PrintTime("Average Time", total_seconds / cli.num_trials());
}


// heuristic to see if sufficently dense power-law graph
bool WorthRelabelling(const CSRGraph &g)
{
  int64_t average_degree = g.num_edges() / g.num_nodes();
  if (average_degree < 10)
    return false;
  SourcePicker<CSRGraph> sp(g);
  int64_t num_samples = std::min(int64_t(1000), g.num_nodes());
  int64_t sample_total = 0;
  pvector<int64_t> samples(num_samples);
  for (int64_t trial = 0; trial < num_samples; trial++)
  {
    samples[trial] = g.out_degree(sp.PickNext());
    sample_total += samples[trial];
  }
  std::sort(samples.begin(), samples.end());
  double sample_average = static_cast<double>(sample_total) / num_samples;
  double sample_median = samples[num_samples / 2];
  return sample_average / 1.3 > sample_median;
}

#endif  // BENCHMARK_H_
