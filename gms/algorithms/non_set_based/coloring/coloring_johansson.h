#include <iterator>
#include <utility>
#include <thread>
#include "coloring_common.h"

namespace GMS::Coloring {

typedef vector<int32_t> ColVec;
typedef ColVec::iterator ColPointer;
typedef vector<ColPointer> PointerVec;

typedef int32_t Color;

// Class that stores the palettes
class Palettes {
public:
    // allocate the memory for the palettes and indecies
    Palettes(size_t n, int32_t max_degree) {
        // use c style arrays because vectors always get inititialized sequentially when created or when using push_back
        delta_plus_two = max_degree + 2;
        // palette of colors still available for the taking
        // first element will be the size of the palette
        palettes = new Color[n * delta_plus_two];

        // index to find if a given color is in the palette, since palette is unordered
        // first element will never be used, as there is no color 0
        // *palette_indecies[v, x] = color x
        palette_indecies = new Color*[n * delta_plus_two];

        // initialize palletes with colors 0..max_degree in parallel
#pragma omp parallel for
        for (NodeId v = 0; v < n; v++) {
            // the initial size of the palette is delta + 1
            palettes[p_ind(v, 0)] = max_degree + 1;

            for (int i = 1; i < delta_plus_two; i++) {
                palette_indecies[p_ind(v, i)] = &palettes[p_ind(v, i)];
                palettes[p_ind(v, i)] = i;
            }
        }
    }

    ~Palettes() {
        delete[] palettes;
        delete[] palette_indecies;
    }

    inline Color at(NodeId v, int32_t pos) {
        return palettes[p_ind(v, pos)];
    }

    // get the size of the palette of node v
    inline size_t size(NodeId v) {
        return palettes[p_ind(v, 0)];
    }

    inline Color* p_at(NodeId v, int32_t pos) {
        return &palettes[p_ind(v, pos+1)];
    }

    inline Color* p_first(NodeId v) {
        return &palettes[p_ind(v, 1)];
    }

    inline Color* p_last(NodeId v) {
        return &palettes[p_ind(v, size(v))];
    }

    // return a pointer to the color c in the palette of node v
    inline Color* p_color(NodeId v, Color c) {
        return palette_indecies[p_ind(v, c)];
    }

    inline bool contains(NodeId v, Color c) {
        return p_color(v, c) <= p_last(v);
    }

    inline void remove_color(NodeId v, Color c) {
        Color* last_pos = p_last(v);
        // update the palette index

        Color* p_color_to_remove = palette_indecies[p_ind(v, c)]; // p_color
        // set the index pointer of the color in question to point to the current end of the palette
        palette_indecies[p_ind(v, c)] = last_pos;
        // set the index pointer of the last color in the palette to position of the color to be removed
        palette_indecies[p_ind(v, *last_pos)] = p_color_to_remove;

        // remove the color picked by the neighbour from own palette and resize palette properly
        *p_color_to_remove = *last_pos;
        *last_pos = c;
        palettes[p_ind(v, 0)]--;
    }

private:
    int32_t* palettes;
    int32_t** palette_indecies;

    int32_t delta_plus_two;

    // returns index of an element in the palette of vertex v
    inline int32_t p_ind(int32_t v, int32_t elem) {
        return delta_plus_two * v + elem;
    }
};

template <class CGraph>
int graph_coloring_johansson_no_updates(const CGraph& g, vector<int32_t>& colors) {
    size_t n = g.num_nodes();

    int32_t max_degree = 0;
    DetailTimer detailTimer;

    // caluculate the max_degree of the graph, in O(log n)
#pragma omp parallel for reduction(max : max_degree)
    for (NodeId v = 0; v < n; v++) {
        if (max_degree < g.out_degree(v)) {
            max_degree = g.out_degree(v);
        }
    }

    // initialize multiple random selectors for concurrent access
    vector<random_selector<>> randoms(omp_get_max_threads());
    for (int32_t i = 0; i < randoms.size(); i++) {
        randoms[i] = random_selector<>();
    }

    // keep track of hom many nodes are colored
    vector<int32_t> nodes_colored(omp_get_max_threads(), 0);
    int32_t nodes_remaining = n;

    detailTimer.endPhase("init");

    int32_t colored = 0;
    int iter = 0;
    // until all nodes are colored...
    while (nodes_remaining > 0) {
#pragma omp parallel
        {
            // select a random color from the node's palette,
            // remember which element that was s.t. we can later see if the coloring took place this round
#pragma omp for schedule(static)
            for (NodeId v = 0; v < n; v++) {
                // move on if this node is already colored
                if (colors[v] > 0)
                    continue;
                // negate the color to indicate that it has been picked this turn
                colors[v] = - randoms[omp_get_thread_num()].select_num(1, max_degree + 1);
            }

            // check if the neighbours also picked that color,
            // if so we don't want to pick the color this round
#pragma omp for schedule(static)
            for (NodeId v = 0; v < n; v++) {
                if (colors[v] > 0)
                    continue;
                for (NodeId u : g.out_neigh(v)) {
                    if (abs(colors[v]) == abs(colors[u])) {
                        colors[v] = 0;
                        break;
                    }
                }

                if(colors[v] != 0){
                    nodes_colored[omp_get_thread_num()]++;
                    colors[v] = abs(colors[v]);
                    continue;
                }
            }
        } // END PARALLEL SECTION
        std::string phaseName("coloring_step_");
        phaseName += std::to_string(iter++);
        detailTimer.endPhase(phaseName.c_str());
        for (int32_t i = 0; i < omp_get_max_threads(); i++) {
            colored += nodes_colored[i];
        }
        nodes_remaining = n - colored;
        colored = 0;
    }

    detailTimer.print();
    return -1;
}

//int graph_coloring_johansson_updates(const CGraph& g, vector<int32_t>& colors, const AlgoParameters& algoParams) {
//    size_t n = g.num_nodes();
//
//    int32_t max_degree = 0;
//    DetailTimer detailTimer;
//
//    // caluculate the max_degree of the graph, in O(log n)
//#pragma omp parallel for reduction(max : max_degree)
//    for (NodeId v = 0; v < n; v++) {
//        if (max_degree < g.out_degree(v)) {
//            max_degree = g.out_degree(v);
//        }
//    }
//
//    Palettes palettes(n, max_degree);
//
//    // initialize multiple random selectors for concurrent access
//    vector<random_selector<>> randoms(omp_get_max_threads());
//    for (int32_t i = 0; i < randoms.size(); i++) {
//        randoms[i] = random_selector<>();
//    }
//
//    // keep track of hom many nodes are colored
//    vector<int32_t> nodes_colored(omp_get_max_threads(), 0);
//    int32_t nodes_remaining = n;
//
//    detailTimer.endPhase("init");
//
//    int32_t colored = 0;
//    int iter = 0;
//    // until all nodes are colored...
//    while (nodes_remaining > 0) {
//#pragma omp parallel
//        {
//            // select a random color from the node's palette,
//            // remember which element that was s.t. we can later see if the coloring took place this round
//#pragma omp for
//            for (NodeId v = 0; v < n; v++) {
//                // move on if this node is already colored
//                if (colors[v] > 0)
//                    continue;
//
//                // node got colored last round, mark it as prior colored and move on
//                if (colors[v] < 0) {
//                    colors[v] = abs(colors[v]);
//                    continue;
//                }
//
//                // select a random element from the palette
//                Color* rand_pointer = randoms[omp_get_thread_num()]
//                        .select_array(palettes.p_first(v), palettes.size(v));
//                // negate the color to indicate that it has been picked this turn
//                colors[v] = - *rand_pointer;
//            }
//
//            // check if the neighbours also picked that color,
//            // if so we don't want to pick the color this round
//#pragma omp for
//            for (NodeId v = 0; v < n; v++) {
//                if (colors[v] > 0)
//                    continue;
//                for (NodeId u : g.out_neigh(v)) {
//                    if (colors[v] == colors[u] && v < u) {
//                        colors[v] = 0;
//                        break;
//                    }
//                }
//            }
//
//            // remove the color that was picked by the node,
//            // if one was picked at all
//#pragma omp for
//            for (NodeId v = 0; v < n; v++) {
//                // if the node got a color prior to this round we skip this step
//                if (colors[v] > 0)
//                    continue;
//                // if the node got a color this round, note that we colored a node and move on
//                if(colors[v] < 0){
//                    nodes_colored[omp_get_thread_num()]++;
//                    continue;
//                }
//                // the node is not colored
//                for (NodeId u : g.out_neigh(v)) {
//                    // neighbour got a color this round, that color is still in our palette
//                    if(colors[u] < 0 && palettes.contains(v, abs(colors[u]))) {
//                        palettes.remove_color(v, abs(colors[u]));
//                    }
//                }
//            }
//        } // END PARALLEL SECTION
//        std::string phaseName("coloring_step_");
//        phaseName += std::to_string(iter++);
//        detailTimer.endPhase(phaseName.c_str());
//        //#pragma omp for reduction(+ : colored)
//        for (int32_t i = 0; i < omp_get_max_threads(); i++) {
//            colored += nodes_colored[i];
//        }
//        nodes_remaining = n - colored;
//        colored = 0;
//    }
//
//    // make sure any node colored in the last iteration is positive
//#pragma omp parallel for
//    for(NodeId v = 0; v < n; v++) {
//        colors[v] = abs(colors[v]);
//    }
//    detailTimer.endPhase("final_abs");
//
//    detailTimer.print();
//    return -1;
//}


template <class CGraph>
int graph_coloring_johansson(const CGraph& g, vector<int32_t>& colors) {
//    string mode = algoParams.get("mode", "noupdates");
//    if(mode == "noupdates") {
        return graph_coloring_johansson_no_updates(g, colors);
//    } else if(mode == "updates") {
//        return graph_coloring_johansson_updates(g, colors, algoParams);
//    }
//    return -1;
}

} // namespace GMS::Coloring