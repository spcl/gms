#include "k_clique_count_set_based.h"

#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/command_line.h"

#include <gms/representations/graphs/set_graph.h>
#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>

using namespace GMS;

// TODO
//What to compare with?
template <typename Set, typename SGraph, typename Set2>
bool CliqueCountVerifier(CSRGraph &g, size_t test_total = 0, size_t k = 4) {
    size_t total = CliqueCount<Set, SGraph, Set2>(g, k);
    std::cout << "acc: " << (float)((float)test_total - total) / total * 100 << "\% error: true " << total << " counted " << test_total << std::endl;
    if (total != test_total)
        std::cout << total << " != " << test_total << std::endl;
    return total == test_total;
}

void PrintCliqueStats(const CSRGraph &g, size_t total_cliques) {
    std::cout << total_cliques << " cliques" << std::endl;
}

int main(int argc, char *argv[]) {
    CLI::Parser parser;
    // TODO formerly allow_relabel
    auto clique_size = parser.add_param("clique-size", "cs", "4", "the clique size");
    auto [args, g] = parser.parse_and_load(argc, argv);
    size_t k = clique_size.to_int();

    BenchmarkKernel(args, g, CliqueCount<RoaringSet, RoaringGraph, RoaringSet>,
                            CliqueCountVerifier<RoaringSet, RoaringGraph,RoaringSet>, k,
                            "RoaringSet", "RoaringGraph");
    
    BenchmarkKernel(args, g, CliqueCount<SortedSet, SetGraph<SortedSetRef>, SortedSetRef>,
                    CliqueCountVerifier<SortedSet, SetGraph<SortedSetRef>, SortedSetRef>, k,
                    "SortedSet", "SortedGraph");

    BenchmarkKernel(args, g, CliqueCount<SortedSet, SortedSetGraph, SortedSet>,
                    CliqueCountVerifier<SortedSet, SortedSetGraph, SortedSet>, k,
                    "SortedSet", "SortedNeighGraph");

    return 0;
}
