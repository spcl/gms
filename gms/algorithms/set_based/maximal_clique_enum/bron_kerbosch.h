#pragma once

#ifndef BKALGORITHMS_H
#define BKALGORITHMS_H

#include <gms/common/types.h>
#include <gms/algorithms/preprocessing/preprocessing.h>

#include "sequential/simple.h"
#include "sequential/eppstein.h"
#include "sequential/tomita.h"
#include "parallel/eppsteinPAR.h"
#include "parallel/EppsteinSubGraph.h"
#include "parallel/EppsteinSubGraphAdaptive.h"

namespace BkSequential
{
template <class SGraph = RoaringGraph>
constexpr auto BkSimpleMce = BkSimple::mce<SGraph>;

template <class SGraph = RoaringGraph>
constexpr auto BkTomitaMce = BkTomita::mce<SGraph>;

template <class SGraph = RoaringGraph>
constexpr auto BkEppsteinMce = BkEppstein::mce<SGraph>;
} // namespace BkSequential

namespace BkParallel
{
template <class SGraph = RoaringGraph>
constexpr auto BkEppsteinDegree = BkEppsteinPar::mce<PpParallel::getDegreeOrdering<SGraph, true>, SGraph>;

template <class SGraph = RoaringGraph>
constexpr auto BkEppsteinDegeneracy = BkEppsteinPar::mce<PpSequential::getDegeneracyOrderingMatula<SGraph, true>, SGraph>;

template <class SGraph>
constexpr auto BkEppsteinSubGraphDegree = BkEppsteinSubGraph::mce<PpParallel::getDegreeOrdering<SGraph, true>, SGraph>;

template <class SGraph>
constexpr auto BkEppsteinSubGraphDegeneracy = BkEppsteinSubGraph::mce<PpSequential::getDegeneracyOrderingMatula<SGraph, true>, SGraph>;

// TODO alias for SubGraphAdaptive

//std::vector<RoaringSet> (&BkEppsteinRecursiveSubGraphDegree)(const RoaringGraph &graph) = BkEppsteinRecursiveSubGraph::mce<PpParallel::getDegreeOrdering>;
//std::vector<RoaringSet> (&BkEppsteinRecursiveSubGraphDegeneracy)(const RoaringGraph &graph) = BkEppsteinRecursiveSubGraph::mce<PpSequential::getDegeneracyOrderingMatulaVoid>;
} // namespace BkParallel

namespace BkParallelBench
{
template <class SGraph = RoaringGraph>
constexpr auto BkEppstein = BkEppsteinPar::mceBench<SGraph>;
}
/*
Templates overview:
--------------------
Expand := BkTomita::expand | BkTomitaPAR::expand
PreProcessing := No | PpParallel::getDegeneracyOrderingMatula | PpParallel::getDegeneracyOrderingApprox
Scheduling := Regular Dynamic | Alternating
*/

#endif