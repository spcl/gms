#include <gms/common/types.h>
#include <gms/algorithms/preprocessing/preprocessing.h>
#include "bench_helper.h"

using namespace GMS::KClique;

template <class CGraph>
using EPPipeline = CliqueCountPipeline<false, CGraph>;

template <class CGraph>
void benchmark_suite(BenchCLApp &cli, CGraph &g) {
    using P = EPPipeline<CGraph>;

    P pipeline(cli);
    pipeline.originalGraph = &g;

    pipeline.SetPrintInfo("ep", "kclisting", "degeneracy");
    pipeline.template Run<P>(cli, &P::Preprocess, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);

    pipeline.SetPrintInfo("ep", "kclisting", "id");
    pipeline.template Run<P>(cli, &P::PreprocessSimple, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);

    pipeline.SetPrintInfo("ep", "kclisting", "degree");
    pipeline.template Run<P>(cli, &P::PreprocessDegree, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);

    std::vector<double> eps = {2, 1, 0.5, 0.1, 0.01, 0.001};
    for(auto e : eps)
    {
        pipeline.epsilon = e;
        char s[21];
        sprintf(s, "approx_%g", e);
        pipeline.SetPrintInfo("ep", "kclisting", s);
        pipeline.template Run<P>(cli, &P::template PreprocessApprox < PpParallel::boundary_function::averageDegree >, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);

        char s2[31];
        sprintf(s2, "approx_MinDegree_%g", e);
        pipeline.SetPrintInfo("ep", "kclisting", s2);
        pipeline.template Run<P>(cli, &P::template PreprocessApprox < PpParallel::boundary_function::minDegree >, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);

        char s3[35];
        sprintf(s3, "approx_ProbMedDegree_%g", e);
        pipeline.SetPrintInfo("ep", "kclisting", s3);
        pipeline.template Run<P>(cli, &P::template PreprocessApprox < PpParallel::boundary_function::probMedianDegree >, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);

        sprintf(s3, "approx_ProbMinDegree%g", e);
        pipeline.SetPrintInfo("ep", "kclisting", s3);
        pipeline.template Run<P>(cli, &P::template PreprocessApprox < PpParallel::boundary_function::probMinDegree >, &P::kclisting, &P::verifierSetup, &P::verify, &P::verifierTearDown);
    }
}

int main(int argc, char *argv[])
{
    auto [cli, g] = parse(argc, argv);

    benchmark_suite(cli, g);

    return 0;
}
