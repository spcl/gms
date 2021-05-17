#ifndef IN_ASCENDING_PERMUTER_H
#define IN_ASCENDING_PERMUTER_H


template <class NodeId_, class DestID_, bool invert>
class InDegreeAscendingPermuter {

public:

    static std::map<NodeId_, NodeId_> permutation_map(CSRGraphBase<NodeId_, DestID_, invert>& graph) {
        std::multimap<int64_t, NodeId_> sorted_map;

        for (auto v : graph.vertices()) {
            int64_t degree = graph.in_degree(v);
            sorted_map.insert(std::pair<int64_t, NodeId_>(degree, v));
        }

        std::map<NodeId_, NodeId_> result;
        NodeId_ new_v = 0;
        for (auto entry : sorted_map) {
            result[entry.second] = new_v;
            new_v++;
        }

        return result;
    }
};


#endif //PROJECT_INORDER_PERMUTER_H
