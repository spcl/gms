#pragma once

#ifndef BRONKERBOSCHHELPER_H
#define BRONKERBOSCHHELPER_H

#include <string>
#include <set>
#include <vector>
#include <sys/sysinfo.h>

#include <iostream>
#include <fstream>
#include <math.h>

unsigned long BK_CLIQUE_COUNTER;

namespace BkHelper
{

template <typename Set>
void printSet(std::string setName, Set &set)
{
    if (!setName.empty())
        setName.append(": ");
    std::cout << setName << "[";
    for (auto ptr = set.begin(); ptr != set.end(); ptr++)
    {
        std::cout << *ptr << ", ";
    }
    std::cout << "]" << std::endl;
}
template <typename Set>
void printSet(Set &set)
{
    printSet("", set);
}

template <typename type>
void printArray(std::string setName, type *arr, int size)
{
    if (!setName.empty())
        setName.append(": ");
    std::cout << setName << "[";
    for (int i = 0; i < size; i++)
    {
        std::cout << arr[i] << ", ";
    }
    std::cout << "]" << std::endl;
}

template <template <class T, class A = std::allocator<T>> class CollectionType, class ItemType>
void printSetOfSet(CollectionType<ItemType> &sets)
{
    std::cout << "Collection:\n"
              << "{" << std::endl;
    for (auto &item : sets)
    {
        std::cout << "\t";
        printSet(item);
    }
    std::cout << "}" << std::endl;
}
template <typename T>
void printSetOfSet(pvector<T> &sets)
{
    std::cout << "Collection:\n"
              << "{" << std::endl;
    for (auto &item : sets)
    {
        std::cout << "\t";
        printSet(item);
    }
    std::cout << "}" << std::endl;
}

template <typename Set>
std::vector<uint32_t> toVector(Set &set)
{
    auto size = set.cardinality();
    std::vector<uint32_t> res(size);
    int counter = 0;
    for (auto v : set)
        res[counter++] = v;

    return res;
}

template <typename Set>
std::vector<std::vector<uint32_t>> toVector(std::vector<Set> &set)
{
    auto size = set.size();
    std::vector<std::vector<uint32_t>> res(size);
    int counter = 0;
    for (auto &inner : set)
    {
        res[counter++] = toVector(inner);
    }

    return res;
}

int getNumberOfAssignedThreads()
{
    int numThreads = 1;
#pragma omp parallel
    {
#pragma omp single
        {
            #ifdef _OPENMP
            int threadnum = omp_get_num_threads();
            #else
            int threadnum = 1;
            #endif
            numThreads = threadnum;
        }
    }
    return numThreads;
}

void printCount()
{
#ifdef BK_COUNT
    std::cout << "The Number of maximal clique counted: " << BK_CLIQUE_COUNTER << std::endl;
#endif
}

void printCountAndReset()
{
#ifdef BK_COUNT
    std::cout << "The Number of maximal clique counted: " << BK_CLIQUE_COUNTER << std::endl;
    BK_CLIQUE_COUNTER = 0;
#endif
}

//Check if BkLib was build with testMacros
bool checkDefTestMacro()
{
#ifdef MINEBENCH_TEST
    return true;
#else
    return false;
#endif
}

//Check if BkLib was build with countMacro
bool checkDefCountMacro()
{
#ifdef BK_COUNT
    return true;
#else
    return false;
#endif
}

//get total free memory
unsigned long get_mem_total_Bytes()
{
    struct sysinfo si;
    sysinfo(&si);
    return si.totalram * si.mem_unit;
}

template <class Graph>
void graphToFile(Graph &g, int scale, bool uniform)
{
    //induce Name
    std::ofstream myfile;
    auto kind = uniform ? "u" : "g";
    myfile.open(kind + std::to_string(scale) + ".el");
    for (int v = 0; v < g.num_nodes(); v++)
    {
        for (auto w : g.out_neigh(v))
        {
            if (w != v)
                myfile << std::to_string(v) + " " + std::to_string(w) + "\n";
        }
    }
    myfile.close();
}
} // namespace BkHelper

#endif //BRONKERBOSCHHELPER_H