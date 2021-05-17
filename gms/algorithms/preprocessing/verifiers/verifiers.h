#pragma once

#include "degeneracy_verifier.h"
#include "gms/algorithms/preprocessing/util/core_number_evaluator.h"

// TODO consolidate like preprocessing.h
namespace PpVerifier
{
    constexpr bool (&DegOrderingVerifier)(const CSRGraph &g, std::vector<NodeId> &result) = DegeneracyOrderingVerifier::degegeneracyOrderingVerifier;
    template <class CGraph = CSRGraph>
    constexpr bool (&DegOrderingApproxVerifier)(const CGraph &g, std::vector<NodeId> &result) = DegeneracyOrderingVerifier::degeneracyOrderingApproxVerifier<CGraph>;
    constexpr bool (&DegreeOrderingVerifier)(const CSRGraph &g, std::vector<NodeId> &result) = DegeneracyOrderingVerifier::degreeOrderingVerifier;
    constexpr CoreNumberEvaluator::CoreNumberInfo (&computeApproxAccuracy)(const std::vector<NodeId> &order, const CSRGraph &g) = CoreNumberEvaluator::evaluateCoreNrAccuracy;
} // namespace PpVerifier