#ifndef GMB_SUBGRAPHISO_VF2_CANDIDATE_GENERATION_H
#define GMB_SUBGRAPHISO_VF2_CANDIDATE_GENERATION_H

#include <vector>
#include <gms/common/types.h>
#include "vf2State.hpp"

namespace GMS::SubGraphIso
{
    //@def get new candidate pair set using T1 and T2
	//@output it will use targetOutFrontier and patternOutFrontier to generate new pairset
	std::vector<std::pair<NodeId, NodeId>> generateCandidatePairSet(State& state)
	{
		std::vector<std::pair<NodeId, NodeId>> candidatePairs;

		// step 1: if target and pattern frontier are not empty: generate all (n,m)
		//   with n in targetOutFrontier and m in patternOutFrontier
		// step 2: if out-frontiers empty: do same for in-frontiers
		// -> symmetric graphs => in-frontier = out-frontier. go to step 3
		// step 3: all pairs (n,m) where n not in targetMappednodes and
		//   m not in patternMappedNodes
		if (state.targetOutFrontier.size() > 0 && state.patternOutFrontier.size() > 0)
		{
			candidatePairs.reserve(state.targetOutFrontier.size());
			// if targetOutFrontier and patternOutFrontier are not empty
			// we generate all pairs of targetOutFrontier and patternOutFrontier

			// any one node from targetOutFrontier which is not in mappingTargetToPattern yet
			for (auto targetNode : state.targetOutFrontier)
			{
				// the first available node in the pattern needs to be mapped anyway
				candidatePairs.push_back(std::make_pair(targetNode, *(state.patternOutFrontier.begin())));
			}
		} else
		{
			// if targetOutFrontier or patternOutFrontier is empty
			// we generate all pair of nodes which are not in M1(s) and M2(s)
			for (NodeId patternIdx = 0; patternIdx < state.patternVertexCount; patternIdx++)
			{
				if (state.mappingPatternToTarget[patternIdx] == -1)
				{
					for (NodeId targetIdx = 0; targetIdx < state.targetVertexCount; targetIdx++)
					{
						if (state.mappingTargetToPattern[targetIdx] == -1)
						{
							candidatePairs.push_back(std::make_pair(targetIdx, patternIdx));
						}
					}

					break;
				}
			}
		}

		return candidatePairs;
	}
}


#endif