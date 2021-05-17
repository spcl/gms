#pragma once

#include <vector>
#include <gms/common/types.h>
#include <gms/third_party/fast_statistics.h>
#include <gms/third_party/fast_range.h>


typedef double (*BoundaryFunction)(const NodeId *vArray, int size, const std::vector<int> &degreeCache,
                                   double epsilon);

namespace PpParallel::boundary_function {

inline double
averageDegree(const NodeId *vArray, int size, const std::vector<int> &degreeCache, double epsilon) {
    double res = 0;
#pragma omp parallel for reduction(+ : res)
    for (int v = 0; v < size; v++) {
        res += degreeCache[vArray[v]];
    }

    return (1 + epsilon) * (res / size);
}

inline double minDegree(const NodeId *vArray, int size, const std::vector<int> &degreeCache, double epsilon) {
    int res = INT32_MAX;
#pragma omp parallel for reduction(min \
                               : res)
    for (int v = 0; v < size; v++)
        res = std::min(res, degreeCache[vArray[v]]);

    return 2 * (1 + epsilon) * res;
}

inline double
probMinDegree(const NodeId *vArray, int size, const std::vector<int> &degreeCache, double epsilon) {
    if (size == 1)
        return degreeCache[vArray[0]];
    if (size == 2)
        return std::min(degreeCache[vArray[0]], degreeCache[vArray[1]]);
    if (size == 3)
        return std::min(std::min(degreeCache[vArray[0]], degreeCache[vArray[1]]), degreeCache[vArray[2]]);

    int res = INT32_MAX;
    int trials = std::max(4, (int) pow(size, 0.5 * (0.001 + (1 - epsilon))));
    //int trials = std::max(4, (int)((1 - epsilon) * sqrt(size)));

#pragma omp parallel
    {
        WyRand::genSeed(omp_get_thread_num());
#pragma omp for reduction(min \
                      : res)
        for (int i = 0; i < trials; i++) {
            res = std::min(res, degreeCache[vArray[fastrange32(WyRand::next(), size)]]);
        }
    }

    return res;
}

inline double
probMedianDegree(const NodeId *vArray, int size, const std::vector<int> &degreeCache, double epsilon) {
    if (size == 1 || size == 2)
        return degreeCache[vArray[0]];
    if (size == 3) {
        int temp[3] = {degreeCache[vArray[0]], degreeCache[vArray[1]], degreeCache[vArray[2]]};
        std::sort(temp, temp + 3);
        return temp[1];
    }

    //int trials = std::max(4, (int)((1 - epsilon) * sqrt(size)));
    int trials = std::max(4, (int) pow(size, 0.5 * (0.001 + (1 - epsilon))));
    std::vector<int> draws(trials);

#pragma omp parallel
    {
        WyRand::genSeed(omp_get_thread_num());
#pragma omp for
        for (int i = 0; i < trials; i++) {
            draws[i] = degreeCache[vArray[fastrange32(WyRand::next(), size)]];
        }
    }

#ifdef _OPENMP
    __gnu_parallel::sort(draws.begin(), draws.end());
#else
    std::sort(draws.begin(), draws.end());
#endif

    return draws[trials / 2];
}

} // PpParallel::boundary_function