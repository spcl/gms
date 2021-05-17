#pragma once

#include "parameter.h"
#include "../format.h"
#include <gms/third_party/gapbs/benchmark.h>
#include <cassert>

namespace GMS::CLI {
    struct GraphSpec {
        std::string name;
        bool is_generator = false;
        // generator specific values
        int64_t gen_scale = -1;
        int64_t gen_avgdeg = -1;
    };

    class Args {
    private:
        std::unordered_map<std::string, Param> params;
    public:
        Args() {
            symmetrize = true;
            verify = false;
            num_trials = 3;
            threads = 0;
            error = 0;
        }

        Args(const std::vector<ParamSpec> &param_specs) : Args() {
            for (const ParamSpec &spec : param_specs) {
                params.insert({spec.name, Param(spec.value_ptr)});
            }
        }

        bool symmetrize;
        bool verify;
        int64_t num_trials;
        int64_t threads;
        GraphSpec graph_spec;
        int error;

        void print() const {
            std::string msg_num_threads = (threads == 0) ? "Use OpenMP default" : std::to_string(threads);

            std::cout
                << "Arguments:" << "\n"
                << "  General Options:" << "\n"
                << "    Verify: " << (verify ? "true" : "false") << "\n"
                << "    Num trials: " << num_trials << "\n"
                << "    Num threads: " << msg_num_threads << "\n"
                << "  Input:" << "\n"
                << "    Source: " << (graph_spec.is_generator ? "Generator" : "File") << "\n"
                << "    Name: " << graph_spec.name << std::endl;

            if (graph_spec.is_generator) {
                std::cout
                    << "    Generator Arguments:" << "\n"
                    << "      Scale: " << graph_spec.gen_scale << "\n"
                    << "      Average Degree: " << graph_spec.gen_avgdeg << std::endl;
            } else {
                std::cout
                    << std::boolalpha
                    << "    Symmetrize: " << symmetrize << "\n";
            }

            if (params.size() > 0) {
                std::cout << "  Benchmark specific parameters:" << "\n";
                for (const auto &p : params) {
                    std::cout << "    " << p.first << ": " << quote_empty_string(p.second.value()) << std::endl;
                }
            }

            print_environment();
        }

        std::optional<Param> param(const std::string &name) const {
            auto search = params.find(name);
            if (search == params.end()) {
                return std::nullopt;
            } else {
                return search->second;
            }
        }

        const Param &param_v(const std::string &name) const {
            auto search = params.find(name);
            assert(search != params.end());
            return search->second;
        }

        // defined in compat.h
        CSRGraph load_graph() const;

    private:
        void print_environment() const {
            std::cout
                << "Compile time settings:" << "\n"
                #ifdef MINEBENCH_TEST
                << "  Test flags: ENABLED (longer execution time)" << "\n"
                #else
                << "  Test flags: disabled" << "\n"
                #endif

                ;

        }
    };
}