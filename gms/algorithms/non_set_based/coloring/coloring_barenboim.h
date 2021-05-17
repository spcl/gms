#ifndef COLORING_BARENBOIM_H_
#define COLORING_BARENBOIM_H_

#include "coloring_common_barenboim_elkin.h"

namespace GMS::Coloring {

template <class CGraph>
std::size_t one_shot_coloring(const CGraph &g, const VN &nodes_to_color, VC &coloring, VVC &palettes, VRS selectors,
                              VC &chosen_color, VC &new_color, std::size_t n_colored) {
    std::size_t n = g.num_nodes();

    // select a color
#pragma omp parallel for schedule(dynamic, 16)
    for (std::size_t i = 0; i < nodes_to_color.size(); i++) {
        NodeId v = nodes_to_color[i];
        if (coloring[v] != 0) {
            continue;
        }
        auto selector = selectors[omp_get_thread_num()];
        chosen_color[v] = selector(palettes[v]);
    }

    // check if we can keep color
    int32_t n_newly_colored = 0;
#pragma omp parallel for schedule(dynamic, 16) reduction(+ : n_newly_colored)
    for (std::size_t i = 0; i < nodes_to_color.size(); i++) {
        NodeId v = nodes_to_color[i];
        if (coloring[v] != 0) {
            continue;
        }

        bool keep_color = true;
        // NodeId last_u = n;
        for (auto it = g.out_neigh(v).end() - 1; it != g.out_neigh(v).begin() - 1; it--) {
            NodeId u = *it;
            // for (NodeId u : std::make_reverse_iterator(g.out_neigh(v))) {}
            // assert(u < last_u && "u decreases strictly");
            // last_u = u;
            // g.out_neigh(v) is sorted, so we can return early
            if (v >= u) {
                break;
            }
            // in a conflict, only keep the color if v has the higher ID
            if (v < u && chosen_color[v] == chosen_color[u]) {
                keep_color = false;
                break;
            }
        }
        if (keep_color) {
            coloring[v] = chosen_color[v];
            new_color[v] = chosen_color[v];
            n_newly_colored++;
        }
    }
    n_colored += n_newly_colored;

    // update palette
#pragma omp parallel for schedule(dynamic, 16)
    for (std::size_t i = 0; i < nodes_to_color.size(); i++) {
        NodeId v = nodes_to_color[i];
        if (coloring[v] != 0) {
            continue;
        }

        for (NodeId u : g.out_neigh(v)) {
            if (new_color[u] != 0) {
                remove_sorted(palettes[v], new_color[u]);
            }
        }
    }

    // set new_color and chosen_color to 0 after a vertex is colored
#pragma omp parallel for schedule(dynamic, 16)
    for (std::size_t i = 0; i < nodes_to_color.size(); i++) {
        NodeId v = nodes_to_color[i];
        if (chosen_color[v] != 0) {
            chosen_color[v] = 0;
        }
        if (new_color[v] != 0) {
            new_color[v] = 0;
        }
    }

    update_palettes(g, coloring, palettes);

    return n_colored;
}

template <class CGraph>
int coloring_barenboim(const CGraph &g, VC &coloring, VN &g_nodes, std::size_t n_colored) {
    VRS selectors(omp_get_max_threads());
    for (std::size_t i = 0; i < selectors.size(); i++) {
        selectors[i] = random_selector<>();
    }

    std::size_t n = g.num_nodes();
    VC chosen_color(n, 0);
    VC new_color(n, 0);

    ColorID delta = get_delta(g);
    VVC palettes = std::move(create_delta_plus_one_palettes(g, coloring, n, delta));
    update_palettes(g, coloring, palettes);

    std::size_t iterations = std::ceil(std::log(delta) / std::log((double)16 / 15));

    for (std::size_t i = 0; i < iterations && n_colored != n; i++) {
        n_colored = one_shot_coloring(g, g_nodes, coloring, palettes, selectors, chosen_color, new_color, n_colored);
    }
    if (n_colored == n) {
        return 0;
    }

    VN g_uncolored = std::move(get_uncolored(g_nodes, coloring));

    const int32_t c = 1;  // TODO
    const int32_t delta_hat = c * std::log(n);

    VN g_hi_nodes(0);
    VN g_lo_nodes(0);

#pragma omp parallel
    {
        VN g_hi_local(0);
        VN g_lo_local(0);

#pragma omp for schedule(dynamic, 16)
        for (std::size_t i = 0; i < g_uncolored.size(); i++) {
            NodeId v = g_uncolored[i];
            int32_t uncolored_deg = 0;
            for (NodeId u : g.out_neigh(v)) {
                uncolored_deg += (coloring[u] == 0);
            }
            if (uncolored_deg > delta_hat) {
                g_hi_local.push_back(v);
            } else {
                g_lo_local.push_back(v);
            }
        }

        if (g_hi_local.size() > 0) {
#pragma omp critical
            push_back_move_all(g_hi_local, g_hi_nodes);
        }

        if (g_lo_local.size() > 0) {
#pragma omp critical
            push_back_move_all(g_lo_local, g_lo_nodes);
        }
    }

    std::sort(g_hi_nodes.begin(), g_hi_nodes.end());
    std::sort(g_lo_nodes.begin(), g_lo_nodes.end());

    iterations = std::ceil(5 * std::log(delta_hat) / std::log((double)4 / 3));

    if (g_hi_nodes.size() > 0) {
        for (std::size_t i = 0; i < iterations && n_colored < n; i++) {
            n_colored =
                    one_shot_coloring(g, g_hi_nodes, coloring, palettes, selectors, chosen_color, new_color, n_colored);
        }
    }

    if (g_lo_nodes.size() > 0) {
        for (std::size_t i = 0; i < iterations && n_colored < n; i++) {
            n_colored =
                    one_shot_coloring(g, g_lo_nodes, coloring, palettes, selectors, chosen_color, new_color, n_colored);
        }
    }

    g_uncolored = std::move(get_uncolored(g_nodes, coloring));

    while (n_colored < n) {
        n_colored =
                one_shot_coloring(g, g_uncolored, coloring, palettes, selectors, chosen_color, new_color, n_colored);
    }

    return n_colored;
}

template <class CGraph>
int coloring_barenboim_subalgo_interface(const CGraph &g, VC &coloring) {
    std::size_t n = g.num_nodes();
    VN g_uncolored = std::move(get_uncolored(g, coloring));
    std::size_t n_colored = n - g_uncolored.size();

    return coloring_barenboim(g, coloring, g_uncolored, n_colored);
}

template <class CGraph>
int coloring_barenboim_direct_interface(const CGraph &g, VC &coloring) {
    std::size_t n = g.num_nodes();
    VN g_uncolored = std::move(get_uncolored(g, coloring));
    std::size_t n_colored = n - g_uncolored.size();

    return coloring_barenboim(g, coloring, g_uncolored, n_colored);
}

} // namespace GMS::Coloring

#endif  // COLORING_BARENBOIM_H_
