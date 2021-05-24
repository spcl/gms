#pragma once

class Fast_RoaringSubGraph
{
public:
    //NOTE: If you want to use this constructor, one should switch to std::vector for vertices instead, since pvcetor.push_back() only copies the values
    //Alternatively one could extend pvector with an emplace_back method which uses move semantics
    // Fast_RoaringSubGraph(const Graph &graph, const NodeId v) : centerVertex(v)
    // {
    //     auto neigh = graph.neigh(v).union_with(RoaringSet(v));
    //     int neighCount = neigh.cardinality();
    //     this->mapping.reserve(neighCount);
    //     this->backReferences.reserve(neighCount);
    //     int counter = 0;
    //     for (auto const w : neigh)
    //     {
    //         this->mapping.insert({w, counter});
    //         this->vertices.push_back(graph.neigh(w).intersect(neigh));
    //         this->backReferences.push_back({this->vertices[counter].cardinality(), w});
    //         counter++;
    //     }
    //     num_nodes_ = neighCount;
    // }

    Fast_RoaringSubGraph(const RoaringGraph &graph, const NodeId v, const RoaringSet &cand, const RoaringSet &fini, const RoaringSet &subg) : centerVertex(v), vertices(subg.cardinality()), backReferences(subg.cardinality())
    {
        int neighCount = subg.cardinality();
        this->mapping.reserve(neighCount);
        int counter = 0;
        for (NodeId const w : cand)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.out_neigh(w).intersect(subg);
            this->backReferences[counter] = {this->vertices[counter].intersect_count(cand), w};
            counter++;
        }
        for (NodeId const w : fini)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.out_neigh(w).intersect(cand);
            this->backReferences[counter] = {this->vertices[counter].cardinality(), w};
            counter++;
        }
        num_nodes_ = neighCount;
    }

    Fast_RoaringSubGraph(const Fast_RoaringSubGraph &graph, const NodeId v, const RoaringSet &cand, const RoaringSet &fini, const RoaringSet &subg) : centerVertex(v), vertices(subg.cardinality()), backReferences(subg.cardinality())
    {
        int neighCount = subg.cardinality();
        this->mapping.reserve(neighCount);
        int counter = 0;
        for (NodeId const w : cand)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.neigh(w).intersect(subg);
            this->backReferences[counter] = {this->vertices[counter].intersect_count(cand), w};
            counter++;
        }
        for (NodeId const w : fini)
        {
            this->mapping.insert({w, counter});
            this->vertices[counter] = graph.neigh(w).intersect(cand);
            this->backReferences[counter] = {this->vertices[counter].cardinality(), w};
            counter++;
        }
        num_nodes_ = neighCount;
    }

    //Call this function only if the graph was scaled down right before
    NodeId findPivot() const
    {
        size_t max = backReferences[0].size;
        NodeId pivot = backReferences[0].label;
        for (int i = 1; i < num_nodes_; i++)
        {
            auto size = backReferences[i].size;
            if (max < size)
            {
                max = size;
                pivot = backReferences[0].label;
            }
        }
        return pivot;
    }

    //TEMP FOR MEASUREMENTS
    NodeId findPivot(const RoaringSet &cand, const RoaringSet &fini) const
    {
        size_t max = backReferences[0].size;
        NodeId pivot = backReferences[0].label;
        for (int i = 1; i < num_nodes_; i++)
        {
            auto size = backReferences[i].size;
            if (max < size)
            {
                max = size;
                pivot = backReferences[i].label;
            }
        }
        return pivot;
    }

    RoaringSet &neigh(NodeId vertex)
    {
        return this->vertices[this->mapping.find(vertex)->second];
    }
    const RoaringSet &neigh(NodeId vertex) const
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
    struct VertexInfo
    {
        size_t size;
        NodeId label;
    };
    pvector<RoaringSet> vertices;
    pvector<VertexInfo> backReferences;
    int64_t num_nodes_;
    NodeId centerVertex;
    robin_hood::unordered_map<NodeId, NodeId> mapping;
};