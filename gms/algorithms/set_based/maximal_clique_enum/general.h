#pragma once

#ifndef BKGENERAL_H
#define BKGENERAL_H

//General Header File for includes
//This is only meant for BKLib, since all algorithms are using the same headers
#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <assert.h>
#include <vector>

#include <omp.h>
#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/builder.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"
#include "gms/third_party/gapbs/pvector.h"
#include <gms/representations/sets/sorted_set.h>
#include <gms/representations/sets/roaring_set.h>
#include <gms/common/types.h>

#include "helper.h"
#include "sub_graph/sub_graph.h"

/*---------- Function typedefs which can be used as template arguments -------------------------- */
//typedef void (*Expand)(RoaringSet cand, RoaringSet fini, RoaringSet Q, std::vector<RoaringSet> &sol, const RoaringGraph &graph);
//typedef std::vector<NodeId> (*PreProcessing)(RoaringGraph graph);
//typedef std::vector<NodeId> (*PreProcessingConst)(const RoaringGraph &graph);
//typedef NodeId (*FindPivot)(const RoaringGraph &graph, const RoaringSet subg, const RoaringSet &cand);

//template <Expand expand>
//using EppBodySchedule = void (*)(const RoaringGraph &rgraph, std::vector<NodeId> &V, std::vector<RoaringSet> &sol);

#endif