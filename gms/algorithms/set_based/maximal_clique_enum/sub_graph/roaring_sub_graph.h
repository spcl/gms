#pragma once

#ifndef ROARINGSUBGRAPH_H
#define ROARINGSUBGRAPH_H

/* ROARING_SUB_GRAPH
This class is used to scale down a Graph. A template is used, since the input graph could be either a RoaringGraph or a RoaringSubGraph.
It could allow even more templated types.
Fast_RoaringSubGraph holds a back references vector additionally. The back references array stores the mapping: vertices index -> original graph label
The reason is, that the function findMax() is used heavily and this way another O(maxDeg(Graph)) scan inside the map can be saved.
The drawbacks are a bigger buildtime and memory consumption.
*/
template <class TSetGraph, class TSet = typename TSetGraph::Set>
class SGraphSubGraph
{
public:
    using Set = TSet;

    //NOTE: If you want to use this constructor, one should switch to std::vector for vertices instead, since pvcetor.push_back() only copies the values
    //Alternatively one could extend pvector with an emplace_back method which uses move semantics
    // RoaringSubGraphGeneric(const Graph &graph, NodeId v) : centerVertex(v)
    // {
    //     auto neigh = graph.neigh(v).union_with(Set(v));
    //     int neighCount = neigh.cardinality();
    //     this->vertices.reserve(neighCount);
    //     this->mapping.reserve(neighCount);
    //     int counter = 0;
    //     for (auto const w : neigh)
    //     {
    //         this->mapping.insert({w, counter++});
    //         this->vertices.push_back(graph.neigh(w).intersect(neigh));
    //     }
    //     num_nodes_ = neighCount;
    // }

    SGraphSubGraph(const TSetGraph &graph, const NodeId v, const Set &cand, const Set &fini) : centerVertex(v), vertices(cand.cardinality() + fini.cardinality())
    {
        RoaringSet subg = cand.union_with(fini);
        int neighCount = vertices.size();
        this->mapping.reserve(neighCount);
        int counter = 0;
        for (auto const w : cand)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.out_neigh(w).intersect(subg);
            counter++;
        }
        for (auto const w : fini)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.out_neigh(w).intersect(cand);
            counter++;
        }
        num_nodes_ = neighCount;
    }

    SGraphSubGraph(const SGraphSubGraph &graph, const NodeId v, const Set &cand, const Set &fini) : centerVertex(v), vertices(cand.cardinality() + fini.cardinality())
    {
        Set subg = cand.union_with(fini);
        int neighCount = vertices.size();
        this->mapping.reserve(neighCount);
        int counter = 0;
        for (auto const w : cand)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.out_neigh(w).intersect(subg);
            counter++;
        }
        for (auto const w : fini)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.out_neigh(w).intersect(cand);
            counter++;
        }
        num_nodes_ = neighCount;
    }

    //Call this function only if the graph was scaled down right before
    NodeId findPivot(const Set &cand, const Set &fini) const
    {

        size_t max = 0;
        NodeId pivot = *cand.begin();

        for (auto v : fini)
        {
            auto size = out_neigh(v).cardinality();
            if (max < size)
            {
                max = size;
                pivot = v;
            }
        }

        for (auto v : cand)
        {
            auto size = out_neigh(v).intersect_count(cand);
            if (max < size)
            {
                max = size;
                pivot = v;
            }
        }

        return pivot;
    }

    Set &out_neigh(NodeId vertex)
    {
        return this->vertices[this->mapping.find(vertex)->second];
    }
    const Set &out_neigh(NodeId vertex) const
    {
        return this->vertices[this->mapping.find(vertex)->second];
    }

    int64_t num_nodes() const
    {
        return num_nodes_;
    }

    NodeId getCenterVertex() const
    {
        return centerVertex;
    }

private:
    pvector<Set> vertices;
    int64_t num_nodes_;
    NodeId centerVertex;
    robin_hood::unordered_map<NodeId, NodeId> mapping;
};



#endif