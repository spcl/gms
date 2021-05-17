#ifndef GMB_SUBGRAPHISO_VF2_FEASIBILITY_RULES_H
#define GMB_SUBGRAPHISO_VF2_FEASIBILITY_RULES_H

#include <algorithm>
#include <vector>
#include <set>

#include <gms/common/types.h>
#include "vf2State.hpp"

namespace GMS::SubGraphIso
{
    // define here some auxiliary functions
    template<typename iterable_t>
	uint intersectionCount(const iterable_t &vector, const std::set<NodeId> &set)
	{
		uint count = 0;
		for (auto const& elem : vector)
		{
			if (set.find(elem) != set.end())
				count++;
		}
		return count;
	}

	//@def get nodes which are not in T or M
	//@output std::set of nodes
	std::set<NodeId> genComplementary(NodeId count, const std::vector<NodeId> &core, const std::set<NodeId> &t)
	{
		std::set<NodeId> res;

		for (NodeId vid = 0; vid < count; vid++)
		{
			if (core[vid] == -1 && t.find(vid) == t.end())
			{
				res.insert(vid);
			}
		}
		return res;
	}

    // ----------------------------------------------------------------------------------------
    // actual feasibility rules


    //@def check if adding (n,m) to graph satisfy rule-(1) and (2)
	//@output true or false
	bool checkCoreRule(const State& state, const CSRGraph &G1, const CSRGraph &G2, const NodeId n, const NodeId m)
	{
		// check from G1 to G2
		//R core (s, u, v) ⇐⇒
		// ∀u' ∈ adj(G1, u) ∩ M1 (s) --> ∃ v' ∈ adj(G2 , v) ∩ M2(s) : (u', v') ∈ M(s)
		// ∧
		// ∀v' ∈ adj(G2, v) ∩ M2(s) --> ∃ u' ∈ adj(G1, u) ∩ M1(s) : (u', v') ∈ M(s)
		for (auto targetNodeNeighbour : G1.out_neigh(n))
		{
			if (state.isInTargetMapping(targetNodeNeighbour))
			{
				bool abort = true;
				for (auto patternNodeNeighbour : G2.out_neigh(m))
				{
					if (state.mappingPatternToTarget[patternNodeNeighbour] == targetNodeNeighbour)
					{
						abort = false;
						break;
					}
				}

				if (abort)
					return false;
			}
		}

		for (auto patternNodeNeighbour : G2.out_neigh(m))
		{
			if (state.isInPatternMapping(patternNodeNeighbour))
			{
				bool abort = true;
				for (auto targetNodeNeighbour : G1.out_neigh(n))
				{
					if (state.mappingTargetToPattern[targetNodeNeighbour] == patternNodeNeighbour)
					{
						abort = false;
						break;
					}
				}
				if (abort)
					return false;
			}
		}
		return true;
	}

    //@def check if adding (n,m) to graph satisfy rule-(3) and (4)
	//@output true or false
	bool checkTermRule(const State& state, const CSRGraph &G1, const CSRGraph &G2, NodeId n, NodeId m)
	{
		uint count1 = intersectionCount(G1.out_neigh(n), state.targetOutFrontier);
		uint count2 = intersectionCount(G2.out_neigh(m), state.patternOutFrontier);
		return count1 >= count2;
	}

    //@def check if adding (n,m) to graph satisfy rule-(5)
	//@output true or false
	bool checkNewRule(const State& state, const CSRGraph &G1, const CSRGraph &G2, NodeId n, NodeId m)
	{
		uint count1 = 0;
		for (auto node : G1.out_neigh(n))
		{
			if (state.mappingTargetToPattern[node] == -1 && state.targetOutFrontier.find(node) == state.targetOutFrontier.end())
			{
				count1++;
			}
		}

		uint count2 = 0;
		for (auto node : G2.out_neigh(m))
		{
			if (state.mappingPatternToTarget[node] == -1 && state.patternOutFrontier.find(node) == state.patternOutFrontier.end())
			{
				count2++;
			}
		}
		return count1 >= count2;
	}

    //@def check all rules
	//@output true or false
	inline bool checkSynRules(const State& state, const CSRGraph &G1, const CSRGraph &G2, NodeId n, NodeId m)
	{
		return checkCoreRule(state, G1, G2, n, m) &&
			   checkTermRule(state, G1, G2, n, m) &&
			   checkNewRule(state, G1, G2, n, m);
	}
}


#endif