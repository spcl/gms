#pragma once

#include <algorithm>
#include <vector>

#include "../general.h"

namespace PpSequential
{
// TODO refactor (parallel version almost equivalent)
template <class CGraph = CSRGraph, class Output = std::vector<NodeId>>
void getDegreeOrdering(const CGraph &g, Output &ranking)
{
    ranking.reserve(g.num_nodes());
    for(NodeId i = 0; i < (NodeId)g.num_nodes(); i++)
    {
        ranking[i] = i;
    }

    class Cmp
    {
        const CGraph &g_;
    public:
        Cmp(const CGraph &g) : g_(g)
        {}
        ~Cmp(){}

        bool cmp(const NodeId& a, const NodeId& b) const
        {
            if (g_.out_degree(a) == g_.out_degree(b))
            {
                return a < b;
            }
            return g_.out_degree(a) > g_.out_degree(b);
        }

        bool operator()(const NodeId& a, const NodeId& b) const
        {
            return cmp(a, b);
        }

    };

    Cmp cmp(g);
    std::sort(ranking.begin(), ranking.end(), cmp);
}
} // namespace PpSequential