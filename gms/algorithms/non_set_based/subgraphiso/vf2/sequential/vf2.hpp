#ifndef GMB_SUBGRAPHISO_VF2_H
#define GMB_SUBGRAPHISO_VF2_H

#include <iostream>
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
		bool solve(const CSRGraph &target, const CSRGraph &pattern, State &state, Result &result);

		void save_result(State &state, Result &result);

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

		// run a solver
		solve(Target, Pattern, state, result);

		return result;
	}

	bool VF2::solve(const CSRGraph &target, const CSRGraph &pattern, State &state, Result &result)
	{

		// if all vertices of pattern are mapped -> found subgraphisomorphism
		if (state.patternMappedNodes.size() == state.patternVertexCount)
		{
			state.SaveToResult(result);
			return true;
		}

		std::vector<std::pair<NodeId, NodeId>> P = generateCandidatePairSet(state);

		// std::cout <<"pairs found are: " << P.size() << std::endl;
		for (std::vector<std::pair<NodeId, NodeId>>::iterator p = P.begin(); p != P.end(); p++)
		{
			const NodeId n = p->first;
			const NodeId m = p->second;

			if (checkSynRules(state, target, pattern, n, m))
			{
				State new_state = state; //make copy of state to allow proper backtracking
				new_state.addNewPair(n, m, target.out_neigh(n), pattern.out_neigh(m));

				if(solve(target, pattern, new_state, result))
					return true;
			}
		}
		return false;
	}
} // namespace GMS::SubGraphIso

#endif