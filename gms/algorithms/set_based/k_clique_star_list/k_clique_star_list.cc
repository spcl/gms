#include <gms/representations/graphs/set_graph.h>
#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>

#include "k_clique_star_list.h"

using namespace GMS;
using namespace GMS::KCliqueStar;
using namespace std::placeholders;

// IMPORTANT: The code currently does not involve the remove-redundancy step and will output duplicate k-clique-stars,
//            so the information should be used only for listing purposes but not counting.

int main(int argc, char *argv[]) {
    CLI::Parser parser;
    auto param_k = parser.add_param("k", std::nullopt, "3", "size k of k-clique-star");
    // TODO previously reorder
    auto [args, g] = parser.parse_and_load(argc, argv);

    int32_t k = param_k.to_int();

    using Set = RoaringSet;

    auto verifier = std::bind(Verify::KCliqueStarsVerifier<Seq::Output<Set, Seq::OutputMode::List>>, _1, _2, k);
    //auto verifier = std::bind(Verify::KCliqueStarsVerifier<Seq::FinalOutput<Set>>, _1, _2, k);

    auto benchList = [k](const RoaringGraph &graph) { return Seq::CliqueStarList(graph, k); };
    BenchmarkKernelBk<RoaringGraph>(args, g, benchList, verifier);

    auto benchParList = [k](const RoaringGraph &graph) { return Par::CliqueStarList(graph, k); };
    BenchmarkKernelBk<RoaringGraph>(args, g, benchParList, verifier);

    return 0;
}
