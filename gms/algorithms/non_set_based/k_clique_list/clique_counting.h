#pragma once

#include <gms/common/types.h>
#include "parallelizationStrategy/SubGraphBuilder.h"
#include "parallelizationStrategy/SubGraphBuilderWInverse.h"
#include "kernels/kclisting.h"
#include "parallelizationStrategy/parallelize.h"
#include "verification/verify.h"

/* Wraps the already templated function calls in function pointers for easy use. */
namespace GMS::KClique::Par
{
    template <class CGraph = CSRGraph>
    constexpr auto NP_kclisting
        = Parallelize::node<Builders::SubGraphBuilder<CGraph>, KcListing<CGraph>, CGraph>;

    template <class CGraph = CSRGraph>
    constexpr auto EP_kclisting
        = Parallelize::edge<Builders::SubGraphBuilder<CGraph>, KcListing<CGraph>, CGraph>;

    template <class CGraph = CSRGraph>
    constexpr auto EPTask_kclisting
        = Parallelize::edge_tasks<Builders::SubGraphBuilder<CGraph>, KcListing<CGraph>, CGraph>;

    template <class CGraph = CSRGraph>
    constexpr auto EPSimple_kclisting
        = Parallelize::edge_simple<Builders::SubGraphBuilder<CGraph>, KcListing<CGraph>, CGraph>;
}

namespace GMS::KClique::Seq
{
    template <class CGraph = CSRGraph>
    constexpr auto Kclisting
        =  Serial::standard<KcListing<CGraph>>;
}