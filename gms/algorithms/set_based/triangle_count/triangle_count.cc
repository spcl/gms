#include "gms/third_party/gapbs/benchmark.h"

#include <gms/common/cli/cli.h>
#include <gms/representations/graphs/set_graph.h>
#include <gms/common/benchmark.h>

#include "triangle_count.h"
#include "verifier.h"

using namespace GMS;
using namespace GMS::TriangleCount;

template <class AnyGraph, class Fn>
constexpr auto output_wrap(Fn fn) {
    return [fn{move(fn)}](const AnyGraph &g) {
        std::vector<int64_t> output;
        fn(g, output);
        return output;
    };
}

template <class SGraph>
void benchmark_suite(CLI::Args &args, const CSRGraph &g, std::string graphName)
{
    auto label = [&](std::string name) {
        return "tc-" + name + "-" + graphName;
    };

    // Total count
    BenchmarkKernelBk<SGraph>(args, g, Seq::count_total<SGraph>, Verify::total_count, label("total-seq"));
    BenchmarkKernelBk<SGraph>(args, g, Par::count_total<SGraph>, Verify::total_count, label("total-par"));

    // Vertex count
    BenchmarkKernelBk<SGraph>(args, g, output_wrap<SGraph>(Seq::vertex_count2<SGraph>), Verify::vertex_count<2>, label("vertex-count2-seq"));
    BenchmarkKernelBk<SGraph>(args, g, output_wrap<SGraph>(Par::vertex_count2<SGraph>), Verify::vertex_count<2>, label("vertex-count2-par"));
    BenchmarkKernelBk<SGraph>(args, g, output_wrap<SGraph>(Par::vertex_count2_once<SGraph>), Verify::vertex_count<2>, label("vertex-count2-once-par"));
}

int main(int argc, char *argv[])
{
    auto [args, g] = CLI::Parser().parse_and_load(argc, argv);

    benchmark_suite<RoaringGraph>(args, g, "RoaringGraph");
    benchmark_suite<SortedSetGraph>(args, g, "SortedSetGraph");
    benchmark_suite<RobinHoodGraph>(args, g, "RobinHoodGraph");

    return 0;
}
