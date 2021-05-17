#ifndef GMB_SUBGRAPHISO_VF2_STATE_H
#define GMB_SUBGRAPHISO_VF2_STATE_H

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "../../util/result.hpp"


namespace GMS::SubGraphIso
{
	struct State
	{
		std::size_t targetVertexCount;  // count of nodes in G1
		std::size_t patternVertexCount; // count of nodes in G2

		//M1 & M2 (nodes already in core)
		std::set<NodeId> targetMappedNodes, patternMappedNodes;

		//T1 and T2 (terminal node sets)
		std::set<NodeId> targetOutFrontier, patternOutFrontier;

		std::vector<NodeId> mappingTargetToPattern, mappingPatternToTarget;

		State(std::size_t count1 = 0, std::size_t count2 = 0);

		template<typename iterable_t>
		void addNewPair(const NodeId targetNode, const NodeId patternNode,
						   const iterable_t &targetNodeNeighbours,
						   const iterable_t &patternNodeNeighbours);
		
		void printMapping();

		Result ToResult() const;
		void SaveToResult(Result& result) const;

		// some small helper functions
		inline bool isInTargetMapping(const NodeId n) const
		{
			return mappingTargetToPattern[n] != -1;
		}

		inline bool isInPatternMapping(const NodeId m) const
		{
			return mappingPatternToTarget[m] != -1;
		}

		inline bool isInTargetFrontier(const NodeId n) const
		{
			return targetOutFrontier.find(n) != targetOutFrontier.end();
		}

		inline bool isInPatternFrontier(const NodeId m) const
		{
			return patternOutFrontier.find(m) != patternOutFrontier.end();
		}
	};

	// constructor
	//@param [in] count - of nodes in pattern graph
	//@param [in] sub - bool indicator if we want to check subgraph isomorphism

	State::State(std::size_t targetVerticesCount, std::size_t patternVerticesCount)
	{
		targetVertexCount = targetVerticesCount;
		patternVertexCount = patternVerticesCount;

		targetMappedNodes.clear(), patternMappedNodes.clear();
		targetOutFrontier.clear(), patternOutFrontier.clear();

		mappingTargetToPattern.resize(targetVerticesCount);
		std::fill(mappingTargetToPattern.begin(), mappingTargetToPattern.end(), -1);

		mappingPatternToTarget.resize(patternVerticesCount);
		std::fill(mappingPatternToTarget.begin(), mappingPatternToTarget.end(), -1);
	}

	template<typename iterable_t>
	void State::addNewPair(const NodeId targetNode, const NodeId patternNode,
						   const iterable_t &targetNodeNeighbours,
						   const iterable_t &patternNodeNeighbours)
	{
		// map nodes
		targetMappedNodes.insert(targetNode);
		patternMappedNodes.insert(patternNode);

		mappingTargetToPattern[targetNode] = patternNode;
		mappingPatternToTarget[patternNode] = targetNode;

		// update frontiers with new neighbours
		for (auto node : targetNodeNeighbours)
		{
			if (mappingTargetToPattern[node] == -1)
				targetOutFrontier.insert(node);
		}

		for (auto node : patternNodeNeighbours)
		{
			if (mappingPatternToTarget[node] == -1)
				patternOutFrontier.insert(node);
		}

		// remove pair from frontier (if they are in frontiers)
		targetOutFrontier.erase(targetNode);
		patternOutFrontier.erase(patternNode);
	}

	//@def print the mapped values
	//@pring
	void State::printMapping()
	{
		printf("%s mapping relationship found:\n", "(Sub-)graph isomorphism");
		for (int i = 0; i < targetVertexCount; i++)
		{
			if (mappingTargetToPattern[i] != -1)
				printf("%d %d\n", i, mappingTargetToPattern[i]);
		}
	}

	Result State::ToResult() const
	{
		Result result;
		if(targetMappedNodes.size() != patternMappedNodes.size() || 
		   patternMappedNodes.size() != patternVertexCount)
		{
			result.aborted = true;
			return result;
		}

		for(NodeId idx = 0; idx < patternVertexCount; idx++)
		{
			result.isomorphism.emplace( mappingPatternToTarget[idx], idx);
		}
		return result;
	}

	void State::SaveToResult(Result& result) const
	{
		if(targetMappedNodes.size() != patternMappedNodes.size() || 
		   patternMappedNodes.size() != patternVertexCount)
		{
			result.aborted = true;
			return;
		}
		result.isomorphism.clear();
		result.aborted = false;

		for(NodeId idx = 0; idx < patternVertexCount; idx++)
		{
			result.isomorphism.emplace( mappingPatternToTarget[idx], idx);
		}
		return;
	}

} // namespace SubGraphIso

#endif