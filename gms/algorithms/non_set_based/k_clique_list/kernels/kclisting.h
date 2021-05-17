#pragma once

#include <cinttypes>
#include <iostream>
#include <cassert>
#include <algorithm>


#include <gms/common/types.h>
#include "gms/third_party/gapbs/gapbs.h"


namespace GMS::KClique
{
    // Slightly improved version of Danisch et al.'s k-clique Listing (DOI: 10.1145/3178876.3186125)
    // Improvements are located mostly in the micro-optimization level and 3- and 4-cliques are counted correctly
    // without them being treated as special case.
    template <class CGraph = CSRGraph>
    class KcListing
    {
    public:
        const int CliqueSize;

    private:
        pvector<uint> _label;
        pvector<pvector<NodeId>> _subGraph;
        pvector<pvector<uint>> _subDegree;
        unsigned long long _count;

        void doCounting()
        {
            //slower variant for direct comparison with kClist_orig
                for(NodeId node : _subGraph[2])
                {
                    // iterate because original problem was listing. Compiler seems to optimize this
                    // anyway away.
                    for(uint neighIdx = 0; neighIdx < _subDegree[2][node]; neighIdx++)
                    {
                        _count++;
                    }
                }
        }

        void buildSubGraph(CGraph& g, const NodeId node, const uint level)
        {
            _subGraph[level-1].clear();
            auto startNh = g.out_neigh(node).begin();
            const uint nodeDegree = _subDegree[level][node];
            
            for(NodeId* neigh = startNh; neigh < startNh + nodeDegree; neigh++)
            {
                
                if(_label[*neigh] == level)
                {
                    _label[*neigh] = level -1;
                    _subGraph[level-1].push_back(*neigh);
                    _subDegree[level-1][*neigh] = 0;
                }
            }
        }

        void orderAndCount(CGraph& g, const uint level)
        {
            for(NodeId innerNode : _subGraph[level-1])
            {
                
                for(auto currNeigh = g.out_neigh(innerNode).begin(),
                    *lastNeigh = currNeigh + _subDegree[level][innerNode];
                    currNeigh < lastNeigh; currNeigh++)
                {
                    if(_label[*currNeigh] == level-1)
                    {
                        _subDegree[level-1][innerNode]++;
                    }
                    else 
                    {
                        // TODO here it currently breaks down for VarintByteBasedGraph
                        std::swap(*(currNeigh--), *(--lastNeigh));
                    }
                }
            }
        }

        void restoreLabels(const uint level)
        {
            for(NodeId node : _subGraph[level-1])
            {
                _label[node] = level;
            }
        }

        void listing(CGraph& g, const uint level)
        {
            // count cliques
            if(level == 2 || level == 1)
            {
                doCounting();
                return;
            }

            // work through all nodes in current subgraph
            for(NodeId node : _subGraph[level])
            {
                // prepare U'
                buildSubGraph(g, node, level);

                // sort adjacency lists and compute degrees
                orderAndCount(g, level);
                listing(g, level-1);

                // restore labels
                restoreLabels(level);
            }
        }

        void init(const uint coreNumber)
        {
            _label.reserve(coreNumber);
            for(int i = 2; i < CliqueSize+1; i++)
            {
                _subGraph[i].reserve(coreNumber);
                _subDegree[i].reserve(coreNumber);
            }

        }

    public:
        KcListing(
            const int cliqueSize, const uint coreNumber)
        : CliqueSize(cliqueSize),
        _label(pvector<uint>(0)),
        _subGraph( pvector<pvector<NodeId>>(CliqueSize+1)),
        _subDegree( pvector<pvector<uint>>(CliqueSize+1)),
        _count(0)
        {
            for(int i = 2; i < CliqueSize+1; i++)
            {
                _subGraph[i] = pvector<NodeId>(0);
                _subDegree[i]= pvector<uint>(0);
            }

            init(coreNumber);
        }

        KcListing(
            const uint cliqueSize,
            CGraph& g)
            : KcListing(cliqueSize, 0)
            {
                uint coreNumber = 0;
                for(uint i = 0; i < g.num_nodes(); i++)
                {
                    coreNumber = coreNumber > g.out_degree(i) ? coreNumber : g.out_degree(i);
                }

                init(coreNumber);
            }

        ~KcListing()
        {
        }

        unsigned long long count(CGraph& g)
        {
            if(CliqueSize == 2) return g.num_edges();
            if(CliqueSize == 1) return g.num_nodes();
            const NodeId nrNodes = g.num_nodes();
            _label.resize(nrNodes);
            for(NodeId i = 0; i < nrNodes; i++)
            {
                _label[i] = CliqueSize;
            }
            for(int i = 2; i < CliqueSize+1; i++)
            {
                _subGraph[i].resize(nrNodes);
                _subDegree[i].resize(nrNodes);
            }

            for(NodeId i = 0; i < nrNodes; i++)
            {
                _subGraph[CliqueSize][i] = i;
                _subDegree[CliqueSize][i] = g.out_degree(i);
            }

            _count = 0;
            listing(g, CliqueSize);
            return _count;
        }
    };

} // namespace GMS::KClique