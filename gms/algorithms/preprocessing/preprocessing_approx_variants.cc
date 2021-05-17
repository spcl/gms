#include <iostream>
#include <vector>
#include <chrono>

#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/pvector.h"

#include "preprocessing.h"

#include <gms/common/cli/cli.h>

using namespace GMS;

int compareApproxVariations(const CSRGraph &g, int trials)
{
    std::cout << "--------------------------------- Comparison of Approx reordering variants ---------------------------------" << std::endl;
    auto t_start = std::chrono::high_resolution_clock::now();
    RoaringGraph rgraph = RoaringGraph::FromCGraph(g);
    auto t_end = std::chrono::high_resolution_clock::now();
    std::cout << "Building RoaringGraph " << std::chrono::duration<double, std::milli>(t_end - t_start).count() / 1000.0 << "\n";
    pvector<NodeId> ordering(rgraph.num_nodes());

    size_t coreNumber, coreNumberOfOrder = 0;

    t_start = std::chrono::high_resolution_clock::now();
    PpSequential::getDegeneracyOrderingMatula(rgraph, ordering);
    t_end = std::chrono::high_resolution_clock::now();
    coreNumber = CoreNumberEvaluator::getCoreNumberOfOrder(ordering, rgraph);
    auto runtime = std::chrono::duration<double, std::milli>(t_end - t_start).count() / 1000.0;
    std::cout << "@@@ Degeneracy-Ordering (Matula) " << runtime << " " << coreNumber << "\n";
    std::cout << "\tRuntime in seconds:\t" << runtime << "\n";
    std::cout << "\tCorenumber of Graph:\t" << coreNumber << "\n";

    t_start = std::chrono::high_resolution_clock::now();
    PpParallel::getDegreeOrdering(rgraph, ordering);
    t_end = std::chrono::high_resolution_clock::now();
    coreNumberOfOrder = CoreNumberEvaluator::getCoreNumberOfOrder(ordering, rgraph);
    runtime = std::chrono::duration<double, std::milli>(t_end - t_start).count() / 1000.0;
    std::cout << "@@@ Degree-Ordering "  << runtime << " " << coreNumberOfOrder << "\n";
    std::cout << "\tRuntime in seconds:\t" << runtime << "\n";
    std::cout << "\tCorenumber of Order:\t" << coreNumberOfOrder << "\n";

    t_start = std::chrono::high_resolution_clock::now();
    PpParallel::getDegeneracyOrderingApproxSGraph<PpParallel::boundary_function::averageDegree, false>(rgraph, ordering, 0.001);
    t_end = std::chrono::high_resolution_clock::now();
    coreNumberOfOrder = CoreNumberEvaluator::getCoreNumberOfOrder(ordering, rgraph);
    runtime = std::chrono::duration<double, std::milli>(t_end - t_start).count() / 1000.0;
    std::cout << "@@@ Approx Degeneracy-Ordering (SGraph)  " << runtime << " " << coreNumberOfOrder << "\n";
    std::cout << "\tRuntime in seconds:\t" << runtime << "\n";
    std::cout << "\tCorenumber of Order:\t" << coreNumberOfOrder << "\n\n";

    std::cout << "Approx Degeneracy-Ordering (CGraph) rigorous evaluation:" << std::endl;
    std::vector<double> epsilon = {0.001, 0.01, 0.1, 0.5, 1};
    auto kernel = {
            std::pair{"@@@ AVG", PpParallel::getDegeneracyOrderingApproxCGraph<PpParallel::boundary_function::averageDegree>},
            std::pair{"@@@ MIN", PpParallel::getDegeneracyOrderingApproxCGraph<PpParallel::boundary_function::minDegree>},
            std::pair{"@@@ PMIN", PpParallel::getDegeneracyOrderingApproxCGraph<PpParallel::boundary_function::probMinDegree>},
            std::pair{"@@@ PMED", PpParallel::getDegeneracyOrderingApproxCGraph<PpParallel::boundary_function::probMedianDegree>}};

    for (auto e : epsilon)
    {
        std::cout << "-------------------------------------\n";
        std::cout << "Epsilon: " << e << "\n";
        std::cout << "-------------------------------------\n";
        for (auto p : kernel)
        {
            t_start = std::chrono::high_resolution_clock::now();
            std::vector<NodeId> order;
            p.second(g, order, e);
            t_end = std::chrono::high_resolution_clock::now();
            double totaltime = std::chrono::duration<double, std::milli>(t_end - t_start).count();

            auto coreInfo = CoreNumberEvaluator::evaluateCoreNrAccuracy(order, rgraph, coreNumber);
            
            for (int i = 1; i < trials; i++)
            {
                t_start = std::chrono::high_resolution_clock::now();
                std::vector<NodeId> order;
                p.second(g, order, e);
                t_end = std::chrono::high_resolution_clock::now();
                totaltime += std::chrono::duration<double, std::milli>(t_end - t_start).count();

                coreInfo.add(CoreNumberEvaluator::evaluateCoreNrAccuracy(order, rgraph, coreNumber));
            }

            coreInfo.scaleDown(trials);
            totaltime /= trials;

            std::cout << "\t\t" << p.first << " " << totaltime / 1000.0 << " ";
            std::cout << e << " " << coreInfo.coreNumberOfOrder << " " << coreInfo.relativeError << " " << coreInfo.faultRate << " " << coreInfo.relativeMeanDifference << "\n";

            std::cout << "\t\t\t Approx Core Number:           " << coreInfo.coreNumberOfOrder << "\n";
            std::cout << "\t\t\t Relative Approximation Error: " << coreInfo.relativeError << "\n";
            std::cout << "\t\t\t Fault Rate:                   " << coreInfo.faultRate << "\n";
            std::cout << "\t\t\t Relative Mean Difference:     " << coreInfo.relativeMeanDifference << "\n";
        }
    }

    return coreNumberOfOrder;
}

int main(int argc, char *argv[]) {
    // TODO formerly allow_relabel
    auto [args, g] = CLI::Parser().parse_and_load(argc, argv);

    compareApproxVariations(g, args.num_trials);
    return 0;
}