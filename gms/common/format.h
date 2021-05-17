#pragma once

#include <string>
#include <sstream>
#include <gms/third_party/gapbs/pvector.h>

namespace GMS {

template <class iterator>
std::string iter2str(iterator begin, iterator end, const std::string& sep = ", ") {
    std::ostringstream result;
    while (begin != end)
    {
        result << *begin;
        ++begin;
        if (begin != end)
        {
            result << sep;
        }
    }
    return result.str();
}

template <class Set>
std::string set2str(const Set& set, const std::string& sep = ", ") {
    return iter2str(set.begin(), set.end(), sep);
}

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

const std::string &quote_empty_string(const std::string &str) {
    static std::string empty("''");
    return str.empty() ? empty : str;
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

    template <class T>
    void printArray(std::string setName, const std::vector<T> &arr) {
        printArray(setName, arr.data(), arr.size());
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

    template <class SGraph, class Set>
    void printSubgraphNeighborhoods(const SGraph &g, const Set &nodes) {
        std::cout << "neighborhoods of subgraph for nodes {" << set2str(nodes) << "}:" << "\n";
        for (auto v : nodes) {
            Set intersection = g.out_neigh(v).intersect(nodes);
            printSet(std::to_string(v), intersection);
        }
    }

} // namespace gms