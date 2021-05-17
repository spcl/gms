#ifndef KCLIB_BUILDER_SUBGRAPHBUILDERWINVERSE_H
#define KCLIB_BUILDER_SUBGRAPHBUILDERWINVERSE_H

#include <cinttypes>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "util.h"
#include "gms/third_party/gapbs/gapbs.h"

namespace GMS::KClique::Builders
{
    /**
     * @brief Offers functionality to extract a subgraph from
     * a graph based on a node/vertex or edge. Adds the inverse
     * edges as well.
     * 
     */
    class SubGraphBuilderWInverse
    {
    public:
        typedef CSRGraph Graph_T;
    private:
        const Graph_T& _origGraph;
        uint _coreNumber;
        SimpleMapping<NodeId> _mapping;
        SimpleMapping<NodeId> _controlMapping;


    public:
        SubGraphBuilderWInverse(const Graph_T& g, const uint coreNumber)
        : _origGraph(g), _coreNumber(coreNumber), _mapping(SimpleMapping<NodeId>(_coreNumber, _origGraph.num_nodes())),
        _controlMapping(SimpleMapping<NodeId>(_coreNumber, _origGraph.num_nodes()))
        {
        }
        ~SubGraphBuilderWInverse() {}

        Graph_T buildSubGraph(NodeId node)
        {
            _mapping.Clear();
            for(NodeId neigh : _origGraph.out_neigh(node))
            {
                _mapping.MapNode(neigh);
            }

            const uint count = _origGraph.out_degree(node);
            NodeId *out_neighs = new NodeId[count*count];
            NodeId **out_index = new NodeId*[count+1];
            NodeId *in_neighs = new NodeId[count*count];
            NodeId **in_index = new NodeId*[count+1];
            
            int cd = 0;
            uint index = 0;
            out_index[index] = &out_neighs[cd];
            for(NodeId currNode : _origGraph.out_neigh(node))
            {
                NodeId currDeg = 0;
                for(NodeId neigh : _origGraph.out_neigh(currNode))
                {
                    if(_mapping.AlreadyMapped(neigh))
                    {
                        out_neighs[cd + currDeg++ ] = _mapping.NewIndex(neigh);
                    }
                }
                cd += currDeg;
                index++;
                out_index[index] = &out_neighs[cd];
            }

            cd = 0;
            index = 0;
            in_index[index] = &in_neighs[cd];
            for(NodeId currNode : _origGraph.out_neigh(node))
            {
                NodeId currDeg = 0;
                for(NodeId neigh: _origGraph.in_neigh(currNode))
                {
                    if(_mapping.AlreadyMapped(neigh))
                    {
                        in_neighs[cd + currDeg++] = _mapping.NewIndex(neigh);
                    }
                }
                cd += currDeg;
                index++;
                in_index[index] = &in_neighs[cd];
            }


            return Graph_T(count, out_index, out_neighs,
                    in_index, in_neighs);
        }

        Graph_T buildSubGraph(NodeId u, NodeId v)
        {
            _controlMapping.Clear();
            _mapping.Clear();
            for(NodeId neigh : _origGraph.out_neigh(u))
            {
                _controlMapping.MapNode(neigh);
            }

            NodeId count = 0;
            NodeId cd = 0;
            for(NodeId neigh : _origGraph.out_neigh(v))
            {
                if(_controlMapping.AlreadyMapped(neigh))
                {
                    _mapping.MapNode(neigh);
                    count++;
                }
            }

            NodeId *out_neighs = new NodeId[count*count]; // rather than cd (more accurate + simpler)
            NodeId **out_index = new NodeId*[count+1];
            NodeId *in_neighs = new NodeId[count*count];
            NodeId **in_index = new NodeId*[count+1];
            
            cd = 0;
            int index = 0;
            out_index[index] = &out_neighs[cd];
            // iterate through neighboours of v, because they
            // induce new index
            for(NodeId currNode : _origGraph.out_neigh(v))
            {
                if(_mapping.AlreadyMapped(currNode))
                {
                    NodeId currDeg = 0;
                    for(NodeId neigh: _origGraph.out_neigh(currNode))
                    {
                        if(_mapping.AlreadyMapped(neigh))
                        {
                            out_neighs[cd + currDeg++] = _mapping.NewIndex(neigh);
                        }
                    }
                    cd += currDeg;
                    index++;
                    out_index[index] = &out_neighs[cd];
                }
            }

            cd = 0;
            index = 0;
            in_index[index] = &in_neighs[cd];
            for(NodeId currNode : _origGraph.out_neigh(v))
            {
                if(_mapping.AlreadyMapped(currNode))
                {
                    NodeId currDeg = 0;
                    for(NodeId neigh: _origGraph.in_neigh(currNode))
                    {
                        if(_mapping.AlreadyMapped(neigh))
                        {
                            in_neighs[cd + currDeg++] = _mapping.NewIndex(neigh);
                        }
                    }
                    cd += currDeg;
                    index++;
                    in_index[index] = &in_neighs[cd];
                }
            }

            return Graph_T(count, out_index, out_neighs,
                    in_index, in_neighs);
        }

        SimpleMapping<NodeId> GetMapping() const
        {
            return _mapping;
        }

        uint CoreNumber() const { return _coreNumber; }
    };

}

#endif