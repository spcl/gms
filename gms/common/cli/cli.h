#pragma once

#include <gms/third_party/gapbs/command_line.h>
#include <gms/third_party/clipp.h>
#include "gms/common/format.h"

#include "parameter.h"
#include "args.h"
#include "compat.h"

namespace GMS::CLI
{
    using clipp::option;
    using clipp::parameter;
    using clipp::value;

    class Parser {
    private:
        std::vector<ParamSpec> param_specs;
        clipp::group custom_params;
        bool allow_directed_ = false;

    public:
        /**
         * @brief Allow directed graphs as input.
         *
         * By default GMS only allows undirected graphs as inputs and symmetrizes any input graphs by default,
         * with this method this functionality can be changed.
         *
         * @param allow can also be set to false with this method again
         */
        void allow_directed(bool allow = true) {
            allow_directed_ = allow;
        }

        /**
         * Define a benchmark specific parameter.
         *
         * @param name         Main identifier of the parameter.
         * @param alias        Alternative identifier of the parameter.
         * @param defaultValue Default value (if none is provided, the parameter is mandatory).
         * @param help         Documentation to be displayed for this parameter.
         * @return an instance of Param which can be used to retrieve the value, after one of the parse
         *   methods has been invoked on this class.
         *   However, for situations where this could be inconvenient, it's also possible to access
         *   the value with the Args.param method.
         */
        Param add_param(
            const std::string &name, const std::optional<std::string> &alias,
            const std::optional<std::string> &defaultValue, const std::string &help)
        {
            ParamSpec param_spec(name, alias, defaultValue, help);
            param_specs.push_back(param_spec);

            std::string help_string;
            if (param_spec.default_value.has_value()) {
                help_string = param_spec.help + " (default: " + quote_empty_string(param_spec.default_value.value()) + ")";
            } else {
                help_string = param_spec.help + " (required)";
            }

            parameter opt = param_spec.alias.has_value()
                            ? option(param_spec.name, param_spec.alias.value())
                            : option(param_spec.name);
            opt.doc(help_string);
            opt.required(!param_spec.default_value.has_value());

            parameter val = value("", *param_spec.value_ptr);

            custom_params.push_back(opt & val);

            return Param(param_specs.back().value_ptr);
        }

        Args parse(int argc, char **argv) const {
            Args args(param_specs);

            std::string file_name;
            std::string gen_name;
            int64_t gen_scale;
            int64_t gen_avgdeg = 16;

            auto cli = (
                option("-v", "--verify").set(args.verify).doc("perform a basic verification of the computation"),
                option("-t", "--threads").doc("specify the number of threads used") & value("threads", args.threads),
                option("-n", "--num-trials").doc("number of iterations for the benchmark") & value("trials", args.num_trials)
            );

            if (custom_params.size() > 0) {
                cli.push_back(
                    option("-p", "--param").doc("set kernel specific parameters") & with_suffix("=", custom_params)
                );
            }

            auto cli_read_file = (
                option("-f", "--file").required(true).doc("read graph from the specified file")
                & value("file_name", file_name)
            );
            if (allow_directed_) {
                cli_read_file.push_back(
                    option("-u", "--undirected", "--no-symmetrize")
                    .set(args.symmetrize, false)
                    .doc("don't symmetrize the input graph before running the benchmark")
                );
            } else {
                // Symmetrize by default.
                args.symmetrize = true;
            }

            auto cli_generate = (
                    option("-g", "--gen").required(true).doc("generate graph with the specified generator")
                    & ( option("uniform").required(true).set(gen_name, std::string("uniform"))
                        | option("kronecker").required(true).set(gen_name, std::string("kronecker"))
                      )
                    & value("scale", gen_scale).doc("size of the generated graph = 2^scale"),
                            option("--deg") & value("average_degree", gen_avgdeg)
            );
            cli.push_back(cli_read_file | cli_generate);

            if (!clipp::parse(argc, argv, cli)) {
                std::cout << make_man_page(cli, argv[0]);
                args.error = 100;
                return args;
            }

            if (!file_name.empty()) {
                args.graph_spec.name = file_name;
                args.graph_spec.is_generator = false;
            } else if (!gen_name.empty()) {
                args.graph_spec.name = gen_name;
                args.graph_spec.is_generator = true;
                args.graph_spec.gen_scale = gen_scale;
                args.graph_spec.gen_avgdeg = gen_avgdeg;
            } else {
                args.error = 101;
                return args;
            }

            // note: copied from gapbs/command_line.h
#ifdef _OPENMP
            if (args.threads != 0) {
                omp_set_dynamic(0);
                omp_set_num_threads(args.threads);
            }
            #pragma omp parallel
            {
                #pragma omp master
                std::cout << "Using " << omp_get_num_threads() << " OMP threads" << std::endl;
            }
#else // _OPENMP
            std::cout << "OMP is disabled. Using 1 thread." << std::endl;
#endif // _OPENMP

            return args;
        }

        auto parse_and_load(int argc, char **argv) const {
            Args args = parse(argc, argv);
            if (args.error != 0) {
                std::exit(args.error);
            }
            args.print();

            auto g = args.load_graph();

            if (!allow_directed_ && g.directed()) {
                // If this happens, it's probably a bug.
                // TODO but check how it interacts with cached/preloaded graphs
                std::cerr << "undirected graph not allowed, but loaded an undirected graph" << std::endl;
                std::exit(100);
            }

            // TODO this should be improved in a further commit
            bool allow_relabel = true;
            if (allow_relabel && WorthRelabelling(g)) {
                g = Builder::RelabelByDegree(g);
                std::cout
                    << "---------\n"
                    << "NOTE: The input graph got relabeled.\n"
                    << "---------" << std::endl;
            }

            return std::make_tuple<Args, CSRGraph>(std::move(args), std::move(g));
        }
    };
}
