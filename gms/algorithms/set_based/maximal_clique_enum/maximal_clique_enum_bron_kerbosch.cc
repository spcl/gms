#include <gms/common/types.h>
#include <gms/representations/graphs/set_graph.h>
#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>

#include "bron_kerbosch.h"
#include "verifier.h"

using namespace GMS;

template <class SGraph = RoaringGraph>
void runSubGraphs(const CLI::Args &args, const CSRGraph &g)
{
    /*std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << "---------------------------------------- BK-GMS-ADR-SG -----------------------------------------------\n";
    BenchmarkKernelBkPP<SGraph>(args, g,
                                preprocessing_bind(PpParallel::getDegeneracyOrderingApproxSGraph<PpParallel::boundary_function::averageDegree, true, SGraph, pvector<NodeId>>, 0.001),
                                BkEppsteinSubGraph::mceBench<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>,
                                "BK-GMS-ADG-SG");
    BkHelper::printCountAndReset();*/

    std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << "---------------------------------------- Eppstein ADG SG-Adaptive-----------------------------------------------\n";
    BenchmarkKernelBkPP<SGraph>(args, g,
                                preprocessing_bind(PpParallel::getDegeneracyOrderingApproxSGraph<PpParallel::boundary_function::averageDegree, true, SGraph, pvector<NodeId>>, 0.001),
                                BkEppsteinSubGraphAdaptive::mceBench<10, SGraph>, BkVerifier::BronKerboschVerifier<SGraph>,
                                "BK-GMS-ADG-S");
    BkHelper::printCountAndReset();
}

template <class SGraph = RoaringGraph>
void runEppstein(const CLI::Args &args, const CSRGraph &g)
{
    std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << "---------------------------------------- Eppstein ADG -----------------------------------------------\n";
    BenchmarkKernelBkPP<SGraph>(args, g,
                                preprocessing_bind(PpParallel::getDegeneracyOrderingApproxSGraph<PpParallel::boundary_function::averageDegree, true, SGraph, pvector<NodeId>>, 0.001),
                                BkEppsteinPar::mceBench<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>,
                                "BK-GMS-ADG");
    BkHelper::printCountAndReset();

    std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << "---------------------------------------- Eppstein Degree -----------------------------------------------\n";
    BenchmarkKernelBkPP<SGraph>(args, g,
                                PpParallel::getDegreeOrdering<SGraph, true, pvector<NodeId>>,
                                BkEppsteinPar::mceBench<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>,
                                "BK-GMS-DEG");
    BkHelper::printCountAndReset();

    std::cout << "---------------------------------------------------------------------------------------------------\n";
    std::cout << "---------------------------------------- Eppstein Degeneracy -----------------------------------------------\n";
    BenchmarkKernelBkPP<SGraph>(args, g,
                                PpSequential::getDegeneracyOrderingMatula<SGraph, true, pvector<NodeId>>,
                                BkEppsteinPar::mceBench<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>,
                                "BK-GMS-DGR");
    BkHelper::printCountAndReset();
}

int main(int argc, char *argv[])
{
    CLI::Parser parser;
    // TODO formerly allow_relabel
    auto [args, g] = parser.parse_and_load(argc, argv);

    if (args.verify)
    {
        if (!BkHelper::checkDefTestMacro())
        {
            if (!BkHelper::checkDefCountMacro())
            {
                std::cout << "Please build with the Test or the Count flag, otherwise the verification is not possible..." << std::endl;
                return 0;
            }
            else
                std::cout << "INFO: Test Flag is not set. Only the clique count number will be verified..." << std::endl;
        }
    }

    if (BkHelper::checkDefCountMacro())
        std::cout << "This build was compiled with the Count flag..." << std::endl;

    std::cout << "---------------------------------------------------------------" << std::endl;
    std::cout << "---------------------- Using RoaringGraph----------------------" << std::endl;
    runEppstein<RoaringGraph>(args, g);
    runSubGraphs<RoaringGraph>(args, g);
    std::cout << "---------------------------------------------------------------" << std::endl;
    std::cout << "---------------------- Using RobinHoodGraph----------------------" << std::endl;
    runEppstein<RobinHoodGraph>(args, g);
    std::cout << "---------------------------------------------------------------" << std::endl;
    std::cout << "---------------------- Using SortedSetGraph----------------------" << std::endl;
    runEppstein<SortedSetGraph>(args, g);
    return 0;
}
