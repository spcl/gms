#pragma once

#include <vector>
#include <gms/third_party/gapbs/graph.h>
#include <gms/representations/sets/sorted_set.h>
#include <gms/representations/sets/sorted_set_ref.h>
#include <gms/representations/sets/roaring_set.h>
#include <gms/representations/sets/robin_hood_set.h>

template <class SetType>
class SetGraph {
public:
    using Set = SetType;
    using SetElement = typename SetType::SetElement;

    explicit SetGraph(size_t num_nodes) :
        neighborhoods(num_nodes), num_nodes_(num_nodes)
    {}

    explicit SetGraph(std::vector<Set> &&neighborhoods):
        neighborhoods(std::move(neighborhoods))
    {
        num_nodes_ = this->neighborhoods.size();
    }

    SetGraph(SetGraph &&) = default;

    SetGraph clone() const {
        std::vector<Set> new_neigh;
        new_neigh.reserve(num_nodes_);
        for (int64_t u = 0; u < num_nodes_; ++u) {
            new_neigh.push_back(neighborhoods[u].clone());
        }
        return SetGraph(std::move(new_neigh));
    }

    SetGraph &operator=(SetGraph &) = delete;
    SetGraph &operator=(const SetGraph &) = delete;
    SetGraph &operator=(SetGraph &&) = default;

    /**
     * Create a SetGraph instance from an edge list.
     *
     * Note that this constructor won't symmetrize the input edge list, if necessary a specialized constructor
     * should be developed.
     *
     * @tparam EL should implement a compatible interface as std::vector<std::pair<NodeId, NodeId>>,
     *            but it can be any type with such an interface.
     * @param edge_list
     * @param num_nodes
     * @param is_sorted
     * @return
     */
    template <class EL>
    static SetGraph FromEL(EL &edge_list, size_t num_nodes, bool is_sorted)
    {
        if (!is_sorted) {
            std::sort(edge_list.begin(), edge_list.end());
        }

        SetGraph graph(num_nodes);

        graph.neighborhoods.reserve(num_nodes);
        auto edge_iterator = edge_list.begin();
        std::vector<SetElement> neigh;
        for (int64_t u = 0; u < num_nodes; ++u) {
            neigh.clear();
            while (edge_iterator != edge_list.end() && edge_iterator->first == u) {
                neigh.push_back(edge_iterator->second);
                ++edge_iterator;
            }
            graph.neighborhoods.push_back(Set(neigh.begin(), neigh.size()));
        }

        return graph;
    }

    /**
     * Create a SetGraph instance from a CGraph graph.
     *
     * @tparam CGraph Type of the input graph
     * @tparam RemoveIsolated If true, isolated vertices will be detected and removed.
     * @param graph Input graph
     * @return
     */
    template <class CGraph, bool RemoveIsolated = false>
    static SetGraph FromCGraph(const CGraph &graph) {
        return SetGraph(cgraph_to_neighborhoods<CGraph, RemoveIsolated>(graph));
    }

    int64_t out_degree(SetElement vertex) const
    {
        return neighborhoods[vertex].cardinality();
    }

    /**
     * Access the neighborhood of the specified vertex.
     *
     * @param vertex NodeId of the vertex in question.
     * @return
     */
    const Set &out_neigh(NodeId vertex) const
    {
        return neighborhoods[vertex];
    }

    /**
     * Mutable neighborhood access is experimental, and might change in the future.
     */
    Set &out_neigh(NodeId vertex)
    {
        return neighborhoods[vertex];
    }

    int64_t num_nodes() const
    {
        return num_nodes_;
    }

    /**
     * Checks whether the graph is directed or not.
     *
     * Note that this method computes this value and the output should be cached if appropriate.
     * This is in contrast to `CSRGraph` which doesn't ever perform this computation, but instead
     * simply uses the symmetrization settings for the property.
     *
     * @return
     */
    bool directed() const {
        for (NodeId u = 0; u < num_nodes_; ++u) {
            for (NodeId v : out_neigh(u)) {
                if (!out_neigh(v).contains(u)) {
                    return true;
                }
            }
        }
        return false;
    }

protected:
    std::vector<Set> neighborhoods;
    int64_t num_nodes_;

private:
    /**
     * Helper function which converts a CGraph to a vector of sets for the neighborhoods.
     * @tparam CGraph
     * @tparam RemoveIsolated
     * @param graph
     * @return
     */
    template <class CGraph, bool RemoveIsolated>
    static auto cgraph_to_neighborhoods(const CGraph &graph) {
        if constexpr (RemoveIsolated) {
            return cgraph_to_neighborhoods_remove_isolated(graph);
        }

        std::vector<Set> neighborhoods;
        int64_t num_nodes = graph.num_nodes();
        neighborhoods.reserve(num_nodes);

        if constexpr (std::is_same_v<CGraph, CSRGraph> && sizeof(SetElement) == sizeof(NodeId)) {
            // Fast version for graph.out_neigh pointers to contiguous memory (i.e. without extra copy):
            for (NodeId u = 0; u < num_nodes; u++) {
                size_t count = graph.out_degree(u);
                SetElement *data = reinterpret_cast<SetElement *>(graph.out_neigh(u).begin());
                neighborhoods.emplace_back(data, count);
            }
        } else {
            // Generic version for graph.out_neigh iterators (requires extra copy):
            std::vector<SetElement> neigh;
            for (NodeId u = 0; u < num_nodes; u++) {
                size_t count = graph.out_degree(u);
                neigh.resize(count);
                std::copy(graph.out_neigh(u).begin(), graph.out_neigh(u).end(), neigh.data());
                neighborhoods.emplace_back(neigh.data(), count);
            }
        }

        return neighborhoods;
    }

    /**
     * Does the same as cgraph_to_neighborhoods, but also removes isolated vertices from the graph.
     *
     * @tparam CGraph
     * @param graph
     * @return
     */
    template <class CGraph>
    static auto cgraph_to_neighborhoods_remove_isolated(const CGraph &graph) {
        int64_t num_nodes = graph.num_nodes();

        std::vector<NodeId> new_labels(num_nodes);
        int64_t num_isolated_vertices = 0;
        for (NodeId i = 0; i < num_nodes; ++i) {
            if (graph.out_degree(i) == 0) {
                new_labels[i] = -1;
                num_isolated_vertices++;
            } else {
                new_labels[i] = i - num_isolated_vertices;
            }
        }

        if (num_isolated_vertices == 0) {
            return cgraph_to_neighborhoods<CGraph, false>(graph);
        }

        std::vector<Set> neighborhoods;
        neighborhoods.reserve(num_nodes - num_isolated_vertices);
        for (int64_t i = 0; i < num_nodes; i++) {
            if (new_labels[i] != -1) {
                size_t count = graph.out_degree(i);

                SetElement *tempNeigh = new SetElement[count];
                std::copy(graph.out_neigh(i).begin(), graph.out_neigh(i).end(), tempNeigh);

                for (int j = 0; j < count; j++) {
                    tempNeigh[j] = new_labels[tempNeigh[j]];
                }

                neighborhoods.emplace_back(tempNeigh, count);
                delete[] tempNeigh;
            }
        }

        std::cout
            << "Removed " << num_isolated_vertices
            << " isolated vertices from the graph, the graph got relabeled and shrunk!" << std::endl;

        return neighborhoods;
    }
};

using SortedSetGraph = SetGraph<SortedSet>;
using RoaringGraph = SetGraph<RoaringSet>;
using RobinHoodGraph = SetGraph<RobinHoodSet>;
