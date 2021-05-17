#include <gms/common/types.h>
#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>
#include <gms/representations/graphs/set_graph.h>

#include "bron_kerbosch.h"
#include "verifier.h"

using namespace GMS;

template <class SGraph>
void benchmark_suite(const CLI::Args &args, const CSRGraph &g, const std::string setgraph_name) {
    std::cout << "===========================> Bron Kerbosch Simple " << setgraph_name << ":" << std::endl;
    BenchmarkKernel(args, g, BkSequential::BkSimpleMce<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>);
    BkHelper::printCount();

    std::cout << "===========================> Bron Kerbosch Tomita " << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(args, g, BkTomita::mceRoaring<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>);
    BkHelper::printCount();

    std::cout << "===========================> Bron Kerbosch Eppstein " << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(args, g, BkEppstein::mceRoaring<SGraph>, BkVerifier::BronKerboschVerifier<SGraph>);
    BkHelper::printCount();
}

int main(int argc, char *argv[])
{
    CLI::Parser parser;
    // TODO formerly allow_relabel
    auto [args, g] = parser.parse_and_load(argc, argv);

    if (args.verify && !BkHelper::checkDefTestMacro()) {
        std::cout << "Please build with the Test flag, otherwise the verification is not possible..." << std::endl;
        return 0;
    }

    if (BkHelper::checkDefCountMacro())
        std::cout << "This build was compiled with the Count flag..." << std::endl;

    benchmark_suite<RoaringGraph>(args, g, "RoaringSet");
    benchmark_suite<SortedSetGraph>(args, g, "SortedSet");
    benchmark_suite<RobinHoodGraph>(args, g, "RobinHoodSet");

    return 0;
}
