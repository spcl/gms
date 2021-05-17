#pragma once
#ifndef GMS_REPRESENTATIONS_GRAPHS_PERMUTERS_H
#define GMS_REPRESENTATIONS_GRAPHS_PERMUTERS_H

#include <gms/representations/graphs/permuters/in_degree_ascending_permuter.h>
#include <gms/representations/graphs/permuters/in_degree_descending_permuter.h>
#include <gms/representations/graphs/permuters/out_degree_ascending_permuter.h>
#include <gms/representations/graphs/permuters/out_degree_descending_permuter.h>

#if CPLEX_ENABLED
#include <gms/representations/graphs/permuters/optimal_diff_nn_ilp_unconstr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_nn_lp_unconstr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_nn_ilp_constr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_nn_lp_constr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_vn_ilp_unconstr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_vn_lp_unconstr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_vn_ilp_constr_permuter.h>
	#include <gms/representations/graphs/permuters/optimal_diff_vn_lp_constr_permuter.h>
	#include <gms/representations/graphs/permuters/o_ilp_nn_un_n_permuter.h>
	#include <gms/representations/graphs/permuters/o_ilp_nn_con_n_permuter.h>
	#include <gms/representations/graphs/permuters/o_ilp_vn_un_n_permuter.h>
	#include <gms/representations/graphs/permuters/o_ilp_vn_con_n_permuter.h>
#endif

enum struct PermuterVariant {
    OutDegreeAscending,
    OutDegreeDescending,
    InDegreeAscending,
    InDegreeDescending,
#ifdef CPLEX_ENABLED
    OptimalDiffNnIlpUnconstr,
    OptimalDiffNnLpUnconstr,
    OptimalDiffNnIlpConstr,
    OptimalDiffNnLpConstr,
    OptimalDiffVnIlpUnconstr,
    OptimalDiffVnLpUnconstr,
    OptimalDiffVnIlpConstr,
    OptimalDiffVnLpConstr,
    OIlpNnUnN,
    OIlpNnConN,
    OIlpVnUnN,
    OIlpVnConN,
#endif
};

#endif // GMS_REPRESENTATIONS_GRAPHS_PERMUTERS_H