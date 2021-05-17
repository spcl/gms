#include <iostream>

#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/command_line.h"

#include "preprocessing.h"

#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>
#include <gms/representations/graphs/coders/varint_byte_based_graph.h>
#include <gms/representations/graphs/kbit_graph.h>

using namespace GMS;
using namespace std::placeholders;


template <class CGraph>
void benchmark_suite_csr(const CGraph &g, const CLI::Args &args, std::string graph_name)
{
    using namespace PpParallel;

    auto label = [&](std::string base) { return "ADG_C_" + base + "_" + graph_name; };

    std::cout << "===========================> ADG AVG 0.01" << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::averageDegree, false, CGraph>, 0.01),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("AVG_0.01"));

    std::cout << "===========================> ADG AVG 0.1 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::averageDegree, false, CGraph>, 0.1),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("AVG_0.1"));
    std::cout << "===========================> ADG AVG 0.5 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::averageDegree, false, CGraph>, 0.5),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("AVG_0.5"));

    std::cout << "===========================> ADG MIN 0.1 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::minDegree, false, CGraph>, 0.1),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("MIN_0.1"));
    std::cout << "===========================> ADG MIN 0.5 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::minDegree, false, CGraph>, 0.5),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("MIN_0.5"));

    std::cout << "===========================> ADG PMIN 0.1 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::probMinDegree, false, CGraph>, 0.1),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("PMIN_0.1"));
    std::cout << "===========================> ADG PMIN 0.5 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
            preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::probMinDegree, false, CGraph>, 0.5),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("PMIN_0.5"));

    std::cout << "===========================> ADG PMEDIAN 0.1 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::probMedianDegree, false, CGraph>, 0.1),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("PMED_0.1"));
    std::cout << "===========================> ADG PMEDIAN 0.5 " << graph_name << std::endl;
    BenchmarkKernel(args, g,
                    preprocessing_wrap_return<CGraph>(getDegeneracyOrderingApproxCGraph<boundary_function::probMedianDegree, false, CGraph>, 0.5),
                    PpVerifier::DegOrderingApproxVerifier<CGraph>, label("PMED_0.5"));
}

template <class SGraph>
void benchmark_suite_setgraph(const CSRGraph &g, const CLI::Args &args, std::string setgraph_name) {
    using namespace PpParallel;

    auto label = [&](const std::string name) {
        return std::string("PP-") + name + "-" + setgraph_name;
    };

    std::cout << "===========================> Degeneracy Matula SEQ SGraph=" << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(args, g,
                              preprocessing_wrap_return<SGraph>(PpSequential::getDegeneracyOrderingMatula<SGraph>),
                              PpVerifier::DegOrderingVerifier, label("Matula-SEQ"));

    std::cout << "===========================> Degeneracy Approx PAR SGraph=" << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(
            args, g, preprocessing_wrap_return<SGraph>(PpParallel::getDegeneracyOrderingApproxSGraph<boundary_function::averageDegree, false, SGraph>, 0.001),
            PpVerifier::DegOrderingApproxVerifier<>, label("ADG-PAR"));

    std::cout << "===========================> Degeneracy Matula PAR SGraph=" << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(args, g, preprocessing_wrap_return<SGraph>(PpParallel::getDegeneracyOrderingMatula<SGraph>), PpVerifier::DegOrderingVerifier,
                              label("Matula-PAR"));

    std::cout << "===========================> Degree Parallel PAR SGraph=" << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(args, g, preprocessing_wrap_return<SGraph>(PpParallel::getDegreeOrdering<SGraph>), PpVerifier::DegreeOrderingVerifier,
                              label("DEG-PAR"));

    // TODO implement verifier
    std::cout << "===========================> Triangle Parallel PAR SGraph=" << setgraph_name << ":" << std::endl;
    BenchmarkKernelBk<SGraph>(args, g, preprocessing_wrap_return<SGraph>(PpParallel::triangleCountOrdering<SGraph>),
                              VerifyUnimplemented,
                              label("TRI-PAR"));
}

int main(int argc, char *argv[])
{
    // TODO formerly allow_relabel
    auto [args, g] = CLI::Parser().parse_and_load(argc, argv);

    benchmark_suite_setgraph<RoaringGraph>(g, args, "RoaringGraph");
    // Note: Commented out since it doesn't provide any performance improvements.
    //benchmark_suite_setgraph<SortedSetGraph>(g, cli, "SortedSetGraph");

    benchmark_suite_csr(g, args, "CSR");

    CLI::GapbsCompat cli(args);
    Builder builder(cli);

    // Examples with compressed graphs.

    // TODO add back after issue #43 is resolved
    //benchmark_suite_csr(builder.csrToVarintByteBased(g), cli, "VarintByteBasedGraph");
    //benchmark_suite_csr(builder.csrToVarintWordBased(g), cli, "VarintWordBasedGraph");
    // TODO there's a misaligned load
    //benchmark_suite_csr(builder.csrToKbit(g), cli, "KBit");

    return 0;
}
