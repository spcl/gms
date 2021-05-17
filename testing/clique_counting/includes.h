#ifndef ABSTRACTIONOPTIMIZING_TESTS_CLIQUECOUNTING_INCLUDES_H
#define ABSTRACTIONOPTIMIZING_TESTS_CLIQUECOUNTING_INCLUDES_H

#define NO_BUILDER_TIME_PRINT

#include <gms/common/types.h>

namespace cc
{
    // TODO refactor this away
    typedef CSRGraph Graph_T;
}

#include <gms/algorithms/non_set_based/k_clique_list/clique_counting.h>
#include <gms/algorithms/preprocessing/preprocessing.h>

#include "UCLApp.h"

#include <gtest/gtest.h>

using namespace GMS::KClique;

#endif