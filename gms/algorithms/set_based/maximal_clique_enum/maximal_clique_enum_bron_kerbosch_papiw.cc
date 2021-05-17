#include <gms/common/types.h>
#include <gms/representations/graphs/set_graph.h>
#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>

#include "bron_kerbosch.h"
#include "verifier.h"

using namespace GMS;

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

    BenchmarkKernelBkPP<RoaringGraph>(args, g,
                                      PpSequential::getDegeneracyOrderingMatula<RoaringGraph, true, pvector<NodeId>>,
                                      BkEppsteinPar::mceBench<RoaringGraph>, BkVerifier::BronKerboschVerifier<RoaringGraph>,
                                      "BK-GMS-DGR");
    BkHelper::printCountAndReset();

    return 0;
}
