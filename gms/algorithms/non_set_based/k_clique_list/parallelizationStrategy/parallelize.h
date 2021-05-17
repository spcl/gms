#pragma once

#include <vector>
#include <omp.h>

#include <gms/common/types.h>
#include "gms/third_party/gapbs/gapbs.h"


#include <gms/common/cli/cli.h>

namespace GMS::KClique {

class CLCliqueApp : public GMS::CLI::GapbsCompat {
protected:
    int clique_size_ = 8;

public:
    CLCliqueApp(const GMS::CLI::Args &args, const GMS::CLI::Param &clique_size) : GMS::CLI::GapbsCompat(args)
    {
        clique_size_ = clique_size.to_int();
    }

    int clique_size() const { return clique_size_;}
};

namespace Parallelize
{
    /**
     * @brief Simple parallelization over the nodes/vertices of a graph
     *
     * @tparam Builder_T Able to construct a subgraph given a graph and a node
     * @tparam Counter_T Able to count the k-cliques within a (sub-)graph
     * @param g The complete graph
     * @param cli Command line interface with relevant parameters
     * @return unsigned long long Total of counted k-cliques
     */
    template<typename Builder_T, typename Counter_T, typename CGraph = CSRGraph>
    unsigned long long node(CGraph& g, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);
        const int cliqueSize = dcli.clique_size();
        if(cliqueSize == 1) return g.num_nodes();
        if(cliqueSize == 2) return g.num_edges();

        uint coreNumber = 0;
        for(NodeId node = 0; node < g.num_nodes(); node++)
        {
            coreNumber = coreNumber > g.out_degree(node) ? coreNumber : g.out_degree(node);
        }

        unsigned long long count = 0;
        #pragma omp parallel reduction(+:count)
        {
            Builder_T builder(g, (uint)coreNumber);
            Counter_T counter(cliqueSize -1, coreNumber);

            #pragma omp for schedule(dynamic, 1) nowait
            for(NodeId node = 0; node < g.num_nodes(); node++)
            {
                CGraph graph = builder.buildSubGraph(node);
                count += counter.count(graph);
            }
        }
        return count;
    }

    /**
     * @brief Parallelization over the edges. Avoids storing an edge list by
     * iterating over the adjacency list of the graph and keeping track of the
     * starting vertex through a counter per thread. Finds the max-degree (=core-
     * number for degeneracy ordering) before starting the actual iteration.
     *
     * @tparam Builder_T Able to construct a subgraph given a graph and two
     * adjacent nodes
     * @tparam Counter_T Able to count the k-cliques within a (sub-)graph
     * @param g The complete graph
     * @param cli Command line interface with relevant parameters
     * @return unsigned long long Total of counted k-cliques
     */
    template<typename Builder_T, typename Counter_T, typename CGraph = CSRGraph>
    unsigned long long edge( CGraph& g, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);
        const int cliqueSize = dcli.clique_size();

        if(cliqueSize == 1) return g.num_nodes();
        if(cliqueSize == 2) return g.num_edges();

        uint coreNumber = 0;
        for(NodeId node = 0; node < g.num_nodes(); node++)
        {
            coreNumber = coreNumber > g.out_degree(node) ? coreNumber : g.out_degree(node);
        }
        unsigned long long count = 0;

        const NodeId first = 0;
        const NodeId last = g.num_nodes() -1;

        #pragma omp parallel reduction(+:count)
        {
            Builder_T builder(g, coreNumber);
            Counter_T counter(cliqueSize-2, coreNumber);

            NodeId u_counter = 0;

            #pragma omp for schedule(dynamic, 1) nowait
            for(auto it = g.out_neigh(first).begin(); it < g.out_neigh(last).end(); it++)
            {
                while(it >= g.out_neigh(u_counter).end())
                {
                    u_counter++;
                }

                CGraph graph = builder.buildSubGraph(u_counter, *it);
                count += counter.count(graph);
            }
        }

        return count;
    }

    /**
     * @brief Parallelization over the edges using omp tasks. Requires one builder
     * and one counter per thread (not per task). Finds the max-degree (=core-
     * number for degeneracy ordering) before starting the actual iteration.
     *
     * @tparam Builder_T Able to construct a subgraph given a graph and two
     * adjacent nodes.
     * @tparam Counter_T Able to count the k-cliques within a (sub-)graph
     * @param g The complete graph
     * @param cli Command line interface with relevant parameters
     * @return unsigned long long Total count of k-cliques
     */
    template<typename Builder_T, typename Counter_T, typename CGraph = CSRGraph>
    unsigned long long edge_tasks(CGraph& g, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);
        const int cliqueSize = dcli.clique_size();

        if(cliqueSize == 1) return g.num_nodes();
        if(cliqueSize == 2) return g.num_edges();

        uint coreNumber = 0;
        for(NodeId node = 0; node < g.num_nodes(); node++)
        {
            coreNumber = coreNumber > g.out_degree(node) ? coreNumber : g.out_degree(node);
        }
        unsigned long long count = 0;
        int length = 1;
        #ifdef _OPENMP
        #pragma omp parallel
        {
            length = omp_get_max_threads();
        }
        #endif
        std::vector<unsigned long long> thread_count = std::vector<unsigned long long>(length, 0);
        std::vector<Builder_T*> builders = std::vector<Builder_T*>(length, nullptr);
        std::vector<Counter_T*> counters = std::vector<Counter_T*>(length, nullptr);

        #pragma omp parallel reduction(+:count)
        {
            #ifdef _OPENMP
            int id = omp_get_thread_num();
            #endif
            #ifndef _OPENMP
            int id = 0;
            #endif

            builders[id] = new Builder_T(g, coreNumber);
            counters[id] = new Counter_T(cliqueSize-2, coreNumber);
            #pragma omp barrier

            #pragma omp single
            {
                #pragma omp task untied
                for(NodeId node = 0; node < g.num_nodes(); node++)
                {
                    for(NodeId neigh : g.out_neigh(node))
                    {
                        #pragma omp task firstprivate(node, neigh)
                        {
                            #ifdef _OPENMP
                            int iid = omp_get_thread_num();
                            #endif
                            #ifndef _OPENMP
                            int iid = 0;
                            #endif
                            CGraph graph = builders[iid]->buildSubGraph(node, neigh);
                            thread_count[iid] += counters[iid]->count(graph);
                        }
                    }
                }
            }

            #pragma omp taskwait
            delete builders[id];
            delete counters[id];

            count += thread_count[id];
        }

        return count;
    }

    /**
     * @brief Simple parallelization over the edges. Very fast parallel
     * loop, but needs to build and store an edge list first.
     *
     * @tparam Builder_T Able to construct a subgraph given a graph and two
     * adjacent nodes
     * @tparam Counter_T Able to count the k-cliques within a (sub-)graph
     * @param g The complete graph
     * @param cli Command line interface with relevant parameters
     * @return unsigned long long Total count of k-cliques
     */
    template<typename Builder_T, typename Counter_T, typename CGraph = CSRGraph>
    unsigned long long edge_simple(CGraph& g, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);
        const int cliqueSize = dcli.clique_size();

        if(cliqueSize == 1) return g.num_nodes();
        if(cliqueSize == 2) return g.num_edges();

        uint coreNumber = 0;

        NodeId* start = new NodeId[g.num_edges()];
        NodeId* target = new NodeId[g.num_edges()];

        size_t idx = 0;
        for(NodeId node = 0; node < g.num_nodes(); node++)
        {
            coreNumber = coreNumber > g.out_degree(node) ? coreNumber : g.out_degree(node);
            for(NodeId neigh : g.out_neigh(node))
            {
                start[idx] = node;
                target[idx] = neigh;
                idx++;
            }
        }
        unsigned long long count = 0;
        int length = 1;
        #ifdef _OPENMP
        {
            length = omp_get_max_threads();
        }
        #endif
        std::vector<unsigned long long> thread_count = std::vector<unsigned long long>(length, 0);

        #pragma omp parallel reduction(+:count)
        {
            Builder_T builder(g, coreNumber);
            Counter_T counter(cliqueSize-2, coreNumber);

            #pragma omp for schedule(dynamic, 1) nowait
            for(size_t i = 0; i < g.num_edges(); i++)
            {
                CGraph graph = builder.buildSubGraph(start[i], target[i]);
                #ifdef _OPENMP
                int id = omp_get_thread_num();
                #endif
                #ifndef _OPENMP
                int id = 0;
                #endif
                thread_count[id] += counter.count(graph);
            }

            #ifdef _OPENMP
            int id = omp_get_thread_num();
            #endif
            #ifndef _OPENMP
            int id = 0;
            #endif
            count += thread_count[id];
        }

        delete[] start;
        delete[] target;
        return count;
    }

    /**
     * @brief Provides mixed parallelization over the edges and nodes of a graph,
     * based on a simple heuristic.
     * IMPORTANT: efficiency/speed is untested
     *
     * @tparam Builder_T Able to construct a subgraph given a graph and
     * either a node or two adjecent nodes
     * @tparam Counter_T Able to count the k-cliques within a (sub-)graph
     * @param cli Command line interface with relevant parameters
     * @param g The complete graph
     * @param coreNumber The corenumber (=max degree in a graph with
     * degeneracy ordering)
     * @return unsigned long long Total count of k-cliques
     */
    template<typename Builder_T, typename Counter_T, typename CGraph = CSRGraph>
    unsigned long long mixed(const CLApp& cli, CGraph& g, int coreNumber=0)
    {
        const int cliqueSize = 1;


        if(cliqueSize == 1) return g.num_nodes();
        if(cliqueSize == 2) return g.num_edges();

        if (coreNumber == 0)
        {
            for(NodeId node = 0; node < g.num_nodes(); node++)
            {
                coreNumber = coreNumber > g.out_degree(node) ? coreNumber : g.out_degree(node);
            }
        }
        unsigned long long count = 0;
        const NodeId first = 0;
        const NodeId last = g.num_nodes() -1;

        #pragma omp parallel reduction(+:count)
        {
            Builder_T builder(g, coreNumber);
            Counter_T counter(cliqueSize -1, coreNumber);

            #pragma omp for schedule(dynamic, 1) nowait
            for(NodeId node = 0; node < g.num_nodes(); node++)
            {
                if(g.out_degree(node) > 3*cliqueSize)
                {
                    for(NodeId neigh : g.out_neigh(node))
                    {
                        #pragma omp task
                        {
                            CGraph graph = builder.buildSubGraph(node, neigh);
                            count += counter.count(graph);
                        }
                    }

                }
                else
                {
                    #pragma omp task
                    {
                        CGraph graph = builder.buildSubGraph(node);
                        count += counter.count(graph);
                    }
                }
            }
        }

        #pragma omp taskwait

        return count;
    }


}

namespace Serial
{
    /**
     * @brief Does not provide any parallelization. Serves as a proxy to have
     * the same structure as with the parallelization routines
     *
     * @tparam Counter_T Able to count the k-cliques within a (sub-)graph
     * @param g The complete graph
     * @param cli Command line interface with relevant parameters
     * @return unsigned long long Total count of k-cliques
     */
    template<typename Counter_T, typename CGraph = CSRGraph>
    unsigned long long standard(CGraph& g, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);
        const int cliqueSize = dcli.clique_size();

        if(cliqueSize == 1) return g.num_nodes();
        if(cliqueSize == 2) return g.num_edges();

        Counter_T counter(cliqueSize, g);
        return counter.count(g);
    }
}

}