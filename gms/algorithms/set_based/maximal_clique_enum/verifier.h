#pragma once

#ifndef BRONKERBOSCHVERIFIER_H
#define BRONKERBOSCHVERIFIER_H

#include "general.h"
#include "helper.h"
#include "sequential/simple.h"

namespace BkVerifier
{
//Struct for comparing two RoaringSets
struct less_than_SetOfSet
{
    inline bool operator()(std::vector<uint32_t> &set1, std::vector<uint32_t> &set2)
    {
        auto set1_size = set1.size();
        auto set2_size = set2.size();
        if (set1_size > set2_size)
            return true;
        else if (set2_size > set1_size)
            return false;
        else
        {
            std::sort(set1.begin(), set1.end());
            std::sort(set2.begin(), set2.end());
            for (unsigned int i = 0; i < set1_size; i++)
            {
                if (set1[i] > set2[i])
                    return true;
                if (set1[i] < set2[i])
                    return false;
            }
            return true;
        }
    }
};

// Compare only the clique count
template <class SGraph, class Set = typename SGraph::Set>
bool BronKerboschCliqueCountVerifier(const CSRGraph &g, size_t cliqueCount)
{
    BkTomita::mce<SGraph>(g);
    if(BK_CLIQUE_COUNTER != cliqueCount)
        return false;
    else
        return true;
}

// Compares with serial implementation
template <class SGraph, class Set = typename SGraph::Set>
bool BronKerboschVerifier(const CSRGraph &g, std::vector<Set> &result)
{
    #ifdef MINEBENCH_TEST
    //Compare result sets
    auto expected = BkSimple::mce<SGraph>(g);
    auto sizeExp = expected.size();
    if (sizeExp != result.size())
        return false;

    auto expectedVec = BkHelper::toVector(expected);
    auto resultVec = BkHelper::toVector(result);

    std::sort(expectedVec.begin(), expectedVec.end(), less_than_SetOfSet());
    std::sort(resultVec.begin(), resultVec.end(), less_than_SetOfSet());

    for (unsigned int v = 0; v < sizeExp; v++)
    {
        if (!(expectedVec[v] == resultVec[v]))
            return false;
    }
    return true;
    #elif defined BK_COUNT
    //Only Compare the clique count
    auto temp = BK_CLIQUE_COUNTER;
    bool correct = BronKerboschCliqueCountVerifier<SGraph>(g, temp);
    BK_CLIQUE_COUNTER = temp;
    return correct;
    #else
        return true;
    #endif
}

} // namespace BkVerifier

#endif