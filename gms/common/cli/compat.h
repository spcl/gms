#pragma once

#include "args.h"

namespace GMS::CLI {
    class GapbsCompat : public BenchCLApp {
    public:
        GapbsCompat(const Args &args) : BenchCLApp(0, {}, "dummy") {
            symmetrize_ = args.symmetrize;
            num_trials_ = args.num_trials;
            do_verify_ = args.verify;
            do_analysis_ = false;
            num_omp_threads_ = args.threads;

            if (args.graph_spec.is_generator) {
                symmetrize_ = true;
                filename_ = "";
                if (args.graph_spec.name == "u" || args.graph_spec.name == "uniform") {
                    uniform_ = true;
                    scale_ = args.graph_spec.gen_scale;
                    degree_ = args.graph_spec.gen_avgdeg;
                } else if (args.graph_spec.name == "g" || args.graph_spec.name == "kronecker") {
                    uniform_ = false;
                    scale_ = args.graph_spec.gen_scale;
                    degree_ = args.graph_spec.gen_avgdeg;
                }
            } else {
                filename_ = args.graph_spec.name;
            }
        }
    };

    CSRGraph Args::load_graph() const {
        GapbsCompat compat(*this);
        Builder b(compat);
        return b.MakeGraph();
    }
}