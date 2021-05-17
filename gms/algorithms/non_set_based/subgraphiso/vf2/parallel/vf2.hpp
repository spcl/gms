#ifndef GMB_SUBGRAPHISO_VF2_THREADED_H
#define GMB_SUBGRAPHISO_VF2_THREADED_H


#include <iostream>
#include <omp.h>
#include <vector>

#include <gms/common/types.h>
#include "../../util/result.hpp"
#include "../util/vf2State.hpp"
#include "../util/candidateGeneration.hpp"
#include "../util/feasibilityRules.hpp"


namespace GMS::SubGraphIso
{
	class VF2
	{
	public:
		const CSRGraph &Target, &Pattern;
		State state;

		VF2(const CSRGraph &target, const CSRGraph &pattern);

		// I tried multiple parallel version. However this parallel version was the best
		bool solve(const CSRGraph &target, const CSRGraph &pattern, State &state, Result &result);

		Result run();
	};

	VF2::VF2(
		const CSRGraph &target,
		const CSRGraph &pattern)
		: Target(target), Pattern(pattern)
	{
		state = State(target.num_nodes(), pattern.num_nodes());
	}

	Result VF2::run()
	{
		//initialize result
		Result result;

		if (Target.num_nodes() < Pattern.num_nodes())
			return result;


		omp_set_nested(true);
		#pragma omp parallel
		{
			#pragma omp taskgroup
			{
				#pragma omp single
				{
					solve(Target, Pattern, state, result);
				} // end of single
			}     // end of taskgroup
		}         // end of parallel
		return result;
	}


	bool VF2::solve(const CSRGraph &target, const CSRGraph &pattern, State &state, Result &result)
	{
		// if M1 is already full stop and return true
		if (state.patternMappedNodes.size() == state.patternVertexCount)
		{
			#pragma omp critical
			state.SaveToResult(result);
			return true;
		}

		std::vector<std::pair<NodeId, NodeId>> P = generateCandidatePairSet(state);

		for (std::vector<std::pair<NodeId, NodeId>>::iterator p = P.begin(); p != P.end(); p++)
		{
			const NodeId n = p->first;
			const NodeId m = p->second;

			if (checkSynRules(state, target, pattern, n, m))
			{
				State new_state = state; // copy of state to allow proper backtracking
				new_state.addNewPair(n, m, target.out_neigh(n), pattern.out_neigh(m));

				bool success = false;
				#pragma omp task default(none) shared(result, std::cout, pattern, target, success) firstprivate(new_state) \
				        if (new_state.targetMappedNodes.size() < omp_get_num_threads())
				{
					#pragma omp cancellation point taskgroup
					bool localSuccess = solve(target, pattern, new_state, result);
					if (localSuccess)
					{
						success |= localSuccess;
						#pragma omp cancel taskgroup
					}
				}

				if(success)
					return true;
			}
		}

		#pragma omp taskwait
		return false;
	}

} // namespace GMS::SubGraphIso

#endif