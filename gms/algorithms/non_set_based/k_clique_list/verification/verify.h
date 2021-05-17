#pragma once

#include <iostream>

#include <gms/third_party/gapbs/benchmark.h>

#include "kclisting_original.h"
#include "kclisting_original_nodeParallel.h"
#include "kclisting_original_edgeParallel.h"
#include "conversion.h"

namespace GMS::KClique::Verifiers
{

    bool Standard(CSRGraph& g, unsigned long long benchmarkCount, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);
        long long cliqueCount = 0;

        class SpecialSparseWrapper
        {
        public:
            specialsparse* g = nullptr;
            int CliqueSize = 0;
            bool doVerify=false;

            SpecialSparseWrapper()
            {}

            ~SpecialSparseWrapper()
            {
                if (doVerify) freespecialsparse(g, CliqueSize);
            }
        };
    

        static SpecialSparseWrapper wrapper = SpecialSparseWrapper();

        if(dcli.do_verify() && !wrapper.doVerify)
        {
            wrapper.g = ToSpecialSparse(g);
            wrapper.doVerify=true;
            wrapper.CliqueSize= dcli.clique_size();
            //ord_core(wrapper.g);
            //relabel(wrapper.g);
            mkspecial(wrapper.g, dcli.clique_size());
        }

        kclique(dcli.clique_size(), wrapper.g, &cliqueCount);

        if( cliqueCount != benchmarkCount)
        {
            std::cout << "Nr of " << dcli.clique_size() << " cliques:" << std::endl;
            std::cout << "Reference: " << cliqueCount << std::endl;
            std::cout << "Benchmark: " << benchmarkCount << std::endl;
        }

        return cliqueCount == benchmarkCount;
    }


    bool EP(EP::graph* g, unsigned long long benchmarkCount, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);

        unsigned long long cliqueCount = EP::kclique_main(dcli.clique_size(), g);
        if( cliqueCount != benchmarkCount)
        {
            std::cout << "Nr of " << dcli.clique_size() << " cliques:" << std::endl;
            std::cout << "Reference: " << cliqueCount << std::endl;
            std::cout << "Benchmark: " << benchmarkCount << std::endl;
        }

        return cliqueCount == benchmarkCount;
    }

    EP::graph* EPSetup(const CSRGraph& ppG, const CSRGraph& g, const CLApp& cli)
    {
        return EP::ToGraph(ppG);
    }

    void EPTearDown(EP::graph* g, const CLApp& cli)
    {
        EP::free_graph(g);
    }

    bool NP(NP::graph* g, unsigned long long benchmarkCount, const CLApp& cli)
    {
        const CLCliqueApp& dcli = dynamic_cast<const CLCliqueApp&>(cli);

        unsigned long long cliqueCount = NP::kclique_main(dcli.clique_size(), g);
        if( cliqueCount != benchmarkCount)
        {
            std::cout << "Nr of " << dcli.clique_size() << " cliques:" << std::endl;
            std::cout << "Reference: " << cliqueCount << std::endl;
            std::cout << "Benchmark: " << benchmarkCount << std::endl;
        }

        return cliqueCount == benchmarkCount;
    }

    NP::graph* NPSetup(const CSRGraph& ppG, const CSRGraph& g, const CLApp& cli)
    {
        return NP::ToGraph(ppG);
    }

    void NPTearDown(NP::graph* g, const CLApp& cli)
    {
        NP::free_graph(g);
    }
}

