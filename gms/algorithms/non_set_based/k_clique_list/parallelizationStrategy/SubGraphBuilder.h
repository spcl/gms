#pragma once

#ifndef KCLIB_BUILDER_SUBGRAPHBUILDEREDGE_H
#define KCLIB_BUILDER_SUBGRAPHBUILDEREDGE_H

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
     * a graph based on a node/vertex or edge
     *
     */
    template <class CGraph = CSRGraph>
    class SubGraphBuilder
    {
    private:
        const CGraph& _origGraph;
        uint _coreNumber;
        SimpleMapping<NodeId> _mapping;
        SimpleMapping<NodeId> _controlMapping;


    public:
        SubGraphBuilder(const CGraph& g, const uint coreNumber)
        : _origGraph(g), _coreNumber(coreNumber), _mapping(SimpleMapping<NodeId>(_coreNumber, _origGraph.num_nodes())),
        _controlMapping(SimpleMapping<NodeId>(_coreNumber, _origGraph.num_nodes()))
        {
        }
        ~SubGraphBuilder() {}

        CGraph buildSubGraph(NodeId node)
        {
            _mapping.Clear();
            for(NodeId neigh : _origGraph.out_neigh(node))
            {
                _mapping.MapNode(neigh);
            }

            const uint count = _origGraph.out_degree(node);
            NodeId *out_neighs = new NodeId[count*count];
            NodeId **out_index = new NodeId*[count+1];
            NodeId *in_neighs = nullptr;
            NodeId **in_index = nullptr;

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

            return construct_graph_helper(count, out_index, out_neighs, in_index, in_neighs);
        }

        CGraph buildSubGraph(NodeId u, NodeId v)
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
            NodeId *in_neighs = nullptr;
            NodeId **in_index = nullptr;

            cd = 0;
            int index = 0;
            out_index[index] = &out_neighs[cd];
            // iterate through neighboours of v, because they
            // induce new index
            for(NodeId currNode : _mapping)
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

            return construct_graph_helper(count, out_index, out_neighs, in_index, in_neighs);
        }

        SimpleMapping<NodeId> GetMapping() const
        {
            return _mapping;
        }

        uint CoreNumber() const { return _coreNumber; }

    private:
        CGraph construct_graph_helper(uint count, NodeId** out_index, NodeId* out_neighs,
                                    NodeId** in_index, NodeId* in_neighs) const
        {
            auto csr_graph = CSRGraph(count, out_index, out_neighs, in_index, in_neighs);

            if constexpr (std::is_same_v<CGraph, CSRGraph>) {
                return csr_graph;
            } else {
                CLApp cli(0, {}, "dummy");
                Builder builder(cli);
                return builder.csrToCGraphGeneric<CGraph>(csr_graph);
            }
        }
    };

}

#endif