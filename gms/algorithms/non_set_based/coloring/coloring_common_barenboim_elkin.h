#ifndef COLORING_BE_COMMON_H_
#define COLORING_BE_COMMON_H_

#include "coloring_common.h"

namespace GMS::Coloring {

typedef std::vector<NodeId> VN;

typedef std::vector<ColorID> VC;
typedef std::vector<VC> VVC;

typedef std::vector<random_selector<>> VRS;

typedef std::vector<int32_t> VI32;


void push_back_move_all(VN &from, VN &to) {
    auto to_size = to.size();
    to.resize(to_size + from.size());
    std::move(from.begin(), from.end(), to.begin() + to_size);
    from.clear();
}

VN get_uncolored(const VN &g_nodes, const VC &coloring) {
    VN g_uncolored(0);

#pragma omp parallel
    {
        VN g_uncolored_local(0);

#pragma omp for schedule(dynamic, 16)
        for (int i = 0; i < g_nodes.size(); i++) {
            NodeId v = g_nodes[i];
            if (coloring[v] == 0) {
                g_uncolored_local.push_back(v);
            }
        }
        std::sort(g_uncolored_local.begin(), g_uncolored_local.end());

        if (g_uncolored_local.size() > 0) {
#pragma omp critical
            push_back_move_all(g_uncolored_local, g_uncolored);
        }
    }

    std::sort(g_uncolored.begin(), g_uncolored.end());
    return g_uncolored;
}

template <class CGraph>
VN get_uncolored(const CGraph &g, const VC &coloring) {
    VN g_uncolored(0);

#pragma omp parallel
    {
        VN g_uncolored_local(0);

#pragma omp for schedule(dynamic, 16)
        for (int v = 0; v < g.num_nodes(); v++) {
            if (coloring[v] == 0) {
                g_uncolored_local.push_back(v);
            }
        }
        std::sort(g_uncolored_local.begin(), g_uncolored_local.end());

        if (g_uncolored_local.size() > 0) {
#pragma omp critical
            push_back_move_all(g_uncolored_local, g_uncolored);
        }
    }

    std::sort(g_uncolored.begin(), g_uncolored.end());
    return g_uncolored;
}

template <class CGraph>
VVC create_delta_plus_one_palettes(const CGraph &g, const VC &coloring, NodeId n, ColorID delta) {
    VC palette_to_copy(delta + 1);
    std::generate(palette_to_copy.begin(), palette_to_copy.end(), [d = 1]() mutable { return d++; });

    VVC palettes(n);
#pragma omp parallel for schedule(dynamic, 16)
    for (NodeId v = 0; v < n; v++) {
        palettes[v] = std::move(VC(delta + 1));
        std::copy(palette_to_copy.begin(), palette_to_copy.end(), palettes[v].begin());
    }

    return palettes;
}


void remove_sorted(VC &palette, ColorID value) {
    auto range = std::equal_range(palette.begin(), palette.end(), value);
    palette.erase(range.first, range.second);
}


ColorID *remove_sorted(ColorID *v_palette_begin, ColorID *v_palette_end, ColorID value) {
    auto range = std::equal_range(v_palette_begin, v_palette_end, value);
    return std::move(range.second, v_palette_end, range.first);
}


template <class CGraph>
void update_palettes(const CGraph &g, const VC &coloring, VVC &palettes) {
#pragma omp parallel for schedule(dynamic, 16)
    for (NodeId v = 0; v < palettes.size(); v++) {
        if (coloring[v] != 0) {
            continue;
        }

        // remove colors of neighbours that were already colored
        for (NodeId u : g.out_neigh(v)) {
            if (coloring[u] != 0) {
                remove_sorted(palettes[v], coloring[u]);
            }
        }
    }
}


template <class CGraph>
ColorID get_delta(const CGraph &g) {
    ColorID delta = 0;
#pragma omp parallel for reduction(max : delta) schedule(dynamic, 16)
    for (NodeId v = 0; v < g.num_nodes(); v++) {
        if (g.out_degree(v) > delta) {
            delta = g.out_degree(v);
        }
    }

    return delta;
}

} // namespace GMS::Coloring

#endif  // COLORING_BE_COMMON_H_
