#ifndef SUBGRAPHISO_UTIL_COMMAND_LINE_H
#define SUBGRAPHISO_UTIL_COMMAND_LINE_H

#include <string>

#include <gms/common/types.h>
#include <gms/common/cli/cli.h>

namespace GMS::SubGraphIso {

class SubGraphIsoCLApp : public GMS::CLI::GapbsCompat
{
protected:
    std::string pattern_filename_ = "";
    int glasgow_k_ = 3;
    int glasgow_l_ = 3;
    bool induced_ = true;

public:
    SubGraphIsoCLApp(const GMS::CLI::Args &args) : GMS::CLI::GapbsCompat(args)
    {
        pattern_filename_ = args.param_v("pattern-file").value();

        auto par_l = args.param("glasgow-l");
        auto par_k = args.param("glasgow-k");
        auto par_ind = args.param("induced");
        glasgow_l_ = 0;
        glasgow_k_ = 0;
        if (par_l.has_value()) glasgow_l_ = par_l.value().to_int();
        if (par_k.has_value()) glasgow_k_ = par_k.value().to_int();
        if (par_ind.has_value()) induced_ = bool(par_ind.value().to_int());
    }

    const std::string &pattern_filename() const { return pattern_filename_;}
    int glasgow_k() const { return glasgow_k_;}
    int glasgow_l() const { return glasgow_l_;}
    bool induced() const { return induced_;}
};

auto subgraph_iso_parse_args(int argc, char **argv, bool has_glasgow_par = false) {
    CLI::Parser parser;
    parser.add_param("pattern-file", "i", "", "Filename of pattern graph to find isomorphisms");
    if (has_glasgow_par) {
        parser.add_param("glasgow-l", "l", "3", "Glasgow Parameter: l");
        parser.add_param("glasgow-k", "K", "3", "Glasgow Parameter: k");
        parser.add_param("induced", "ind", "1", "Solve induced SGI");
    }

    auto args = parser.parse(argc, argv);

    if (args.error != 0) {
        std::exit(args.error);
    }

    args.print();

    return SubGraphIsoCLApp(args);
}

} // namespace GMS::SubGraphIso

#endif