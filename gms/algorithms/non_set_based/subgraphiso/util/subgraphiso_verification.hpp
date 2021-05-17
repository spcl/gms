#include "result.hpp"

#include <vector>
#include <algorithm>
#include <set>

#include <gms/common/types.h>

namespace GMS::SubGraphIso
{
    bool subgraphisomorphismVerification(Result result, CSRGraph& target, CSRGraph& pattern,
                                         bool isInducedIsomorphism = true)
    {
        // testing for an isomorphism in a trivial way in large
        // graphs takes a prohibitive amount of time
        if(result.isomorphism.size() == 0 || result.aborted)
            return true;

        // idea: build edge lists of patterngraph and
        // of mapped nodes in target graph
        // if the isomorphism is induced: the edgelists must be the same
        // if it is not induced, the the patternEdgeList is a subset of the
        // targetEdgeList
           
        std::vector<NodeId> targetMap(target.num_nodes(), -1);
        std::vector<NodeId> patternMap(pattern.num_nodes(), -1);
        for(auto pair : result.isomorphism)
        {
            targetMap[pair.first] = pair.second;
            patternMap[pair.second] = pair.first;
        }

        //pair of ints have default comparison: p1.first < p2.first ? true : p1.second < p2.second;
        std::set<std::pair<NodeId, NodeId>> targetEdgeSet;
        std::set<std::pair<NodeId, NodeId>> patternEdgeSet;
        for(NodeId node = 0; node < pattern.num_nodes(); node++)
        {
            for(auto neigh : pattern.out_neigh(node))
            {
                patternEdgeSet.insert(std::make_pair(node, neigh));
            }
        }

        for(NodeId node = 0; node < target.num_nodes(); node++)
        {
            if(targetMap[node] != -1)
            {
                for(auto neigh : target.out_neigh(node))
                {
                    if(targetMap[neigh] != -1)
                    {
                        targetEdgeSet.insert(std::make_pair(targetMap[node], targetMap[neigh]));
                    }
                }
            }
        }

        for(auto iter : patternEdgeSet)
        {
            if(targetEdgeSet.find(iter) == targetEdgeSet.end())
            {
                return false; // pattern contains non-mapped edgee
            }
        }

        if(isInducedIsomorphism)
        {
            for(auto iter : targetEdgeSet)
            {
                if(patternEdgeSet.find(iter) == patternEdgeSet.end())
                {
                    return false; //mapping is not induced isomorphism
                }
            }
        }

        return true;
    }
}