#ifndef ABSTRACTIONOPTIMIZING_MINEBENCH_CLIQUECOUNTING_AUXIL_CONVERSION_H
#define ABSTRACTIONOPTIMIZING_MINEBENCH_CLIQUECOUNTING_AUXIL_CONVERSION_H

#include <set>

#include "gms/third_party/gapbs/gapbs.h"
#include <gms/common/types.h>
#include "kclisting_original.h"
#include "kclisting_original_edgeParallel.h"
#include "kclisting_original_nodeParallel.h"

namespace GMS::KClique {

specialsparse* ToSpecialSparse(const CSRGraph& g)
{
    using SetElement_T = NodeId;

    specialsparse* sparse = (specialsparse*)malloc(sizeof(specialsparse));
    sparse->n = g.num_nodes();
    sparse->e = g.num_edges();

    sparse->edges = (edge*)malloc(sparse->e*sizeof(edge));

    unsigned ccount = 0; //cumulative count of edges
    if(g.directed())
    {
        for(unsigned i = 0; i < g.num_nodes(); i++)
        {
            typename CSRGraph::Neighborhood_t x = g.out_neigh(i);
            for(SetElement_T elem : x)
            {
                edge new_edge;
                new_edge.s = i;
                new_edge.t = elem;
                sparse->edges[ccount] = new_edge;
                ccount++;
            }
        }
    }
    else
    {
        std::set<unsigned int> loopIndices = std::set<unsigned int>();
        for(unsigned i = 0; i < g.num_nodes(); i++)
        {
            typename CSRGraph::Neighborhood_t x = g.out_neigh(i);
            for(SetElement_T elem : x)
            {
                // avoid counting edges twice
                if(i < (unsigned)elem)
                {
                    edge new_edge;
                    new_edge.s = i;
                    new_edge.t = elem;
                    sparse->edges[ccount] = new_edge;
                    ccount++;
                }
                // handle loops
                if(i == (unsigned)elem)
                {
                    if(loopIndices.find(i) != loopIndices.end())
                    {
                        edge new_edge;
                        new_edge.s = i;
                        new_edge.t = elem;
                        sparse->edges[ccount] = new_edge;
                        ccount++;
                        loopIndices.insert(i);
                    }
                }
            }
        }
    }

    return sparse;
}
}

namespace GMS::KClique::EP
{
    template <class CGraph>
    graph* ToGraph(const CGraph &g)
    {
        using SetElement_T = NodeId;

        graph* gg = (graph*)malloc(sizeof(graph));
        gg->cd = (NodeId*)malloc((g.num_nodes()+1)*sizeof(NodeId));
        gg->cd[0] = 0;
        uint max = 0;

        for(int i = 1; i < g.num_nodes()+1; i++)
        {
            gg->cd[i] = gg->cd[i-1]+g.out_degree(i-1);
            max = max > g.out_degree(i-1) ? max : g.out_degree(i-1);
        }

        gg->adj = (NodeId*)malloc(g.num_edges()*sizeof(NodeId));
        gg->edges = (edge*)malloc(g.num_edges()*sizeof(edge));
        NodeId cd = 0;
        for(SetElement_T node = 0; node < g.num_nodes(); node++)
        {
            for(SetElement_T neigh : g.out_neigh(node))
            {
                gg->adj[ cd ] = (NodeId)neigh;
                edge new_edge;
                new_edge.s = (NodeId)node;
                new_edge.t = (NodeId)neigh;
                gg->edges[cd] = new_edge;

                cd++;
            }
        }

        gg->core = max;
        gg->n = g.num_nodes();
        gg->e = g.num_edges();
        return gg;
    }
}

namespace GMS::KClique::NP
{
    template <class CGraph>
    graph* ToGraph(const CGraph& g)
    {
        using SetElement_T = NodeId;

        graph* gg = (graph*)malloc(sizeof(graph));
        gg->cd = (NodeId*)malloc((g.num_nodes()+1)*sizeof(NodeId));
        gg->cd[0] = 0;
        NodeId max = 0;

        for(SetElement_T i =1; i < g.num_nodes()+1; i++)
        {
            gg->cd[i] = gg->cd[i-1]+g.out_degree(i-1);
            max = max > (NodeId)g.out_degree(i-1) ? max : (NodeId)g.out_degree(i-1);
        }

        gg->adj = (NodeId*)malloc(g.num_edges()*sizeof(NodeId));
        NodeId cd = 0;
        for(SetElement_T node = 0; node < g.num_nodes(); node++)
        {
            for(SetElement_T neigh : g.out_neigh(node))
            {
                gg->adj[cd++] = (NodeId)neigh;
            }
        }

        gg->core=max;
        gg->n = g.num_nodes();

        return gg;
    }
}

#endif

