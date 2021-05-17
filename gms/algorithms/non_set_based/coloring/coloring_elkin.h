#ifndef COLORING_ELKIN_H_
#define COLORING_ELKIN_H_

#include "coloring_common_barenboim_elkin.h"
#include "coloring_barenboim.h"

namespace GMS::Coloring {

template <class CGraph>
int coloring_elkin(const CGraph &g, VC &coloring, VN &g_nodes, std::size_t n_colored) {
    NodeId n = g.num_nodes();

    VRS selectors(omp_get_max_threads());
    for (std::size_t i = 0; i < selectors.size(); i++) {
        selectors[i] = random_selector<>();
    }

    ColorID delta = get_delta(g);

    // requirement: epsilon * delta = \Omega(log^2 n)
    double epsilon = std::pow(std::log(n), 2) / delta;
    // requirement: 0 < epsilon < 1
    while (epsilon >= 1) {
        epsilon /= 2;
    }
    while (epsilon < 0.5) {
        epsilon *= 2;
    }
    double epsilon_delta = epsilon * delta;

    // https://www.wolframalpha.com/input/?i=solve+%7B+%28eps*delta%29%5E%281-gamma%29+%3D+ln+n%2C+gamma%3E0+%7D+for+gamma
    double gamma = std::log(delta * epsilon / std::log(n)) / std::log(delta * epsilon);

    VVC palettes = std::move(create_delta_plus_one_palettes(g, coloring, n, delta));
    update_palettes(g, coloring, palettes);

    double d_i = delta;
    double t = std::pow(epsilon_delta, 1 - gamma);
    auto alpha = [epsilon_delta](double d_i) { return std::exp(-(d_i + epsilon_delta) / (8 * (d_i + 1))); };
    auto d_next = [t, epsilon_delta, alpha](double d_i) {
        if (d_i > t) {
            return std::max(1.01 * alpha(d_i) * d_i, t);
        }
        return t / epsilon_delta * d_i;
    };

    VVC chosen_colors(n);
    VC new_color(n, 0);

    bool made_progress = true;

    while (n_colored < g_nodes.size() && made_progress) {
        // every node will compute it's own p_i from that
        double p_i_precompute = (d_i + epsilon_delta) / (d_i + 1);
        d_i = d_next(d_i);

        // choose all palettes
#pragma omp parallel for schedule(dynamic, 16)
        for (std::size_t j = 0; j < g_nodes.size(); j++) {
            NodeId v = g_nodes[j];
            // no action for colored nodes
            if (coloring[v] != 0) {
                continue;
            }

            auto selector = selectors[omp_get_thread_num()];

            auto palette_v_size = palettes[v].size();
            double p_i = p_i_precompute / palette_v_size;

            chosen_colors[v] = std::move(VC());
            // reserve the expected amount of colors be chosen
            chosen_colors[v].reserve(p_i_precompute);  // p_i_precompute == palette_v_size * p_i
            // choose each color from its palette with probability p_i
            for (std::size_t k = 0; k < palette_v_size; k++) {
                // choose each color with probability p_i
                if (selector.rand_0_1() < p_i) {
                    chosen_colors[v].push_back(palettes[v][k]);
                }
            }
        }

        // check if we can keep color
        int32_t n_newly_colored = 0;
#pragma omp parallel for schedule(dynamic, 16) reduction(+ : n_newly_colored)
        for (std::size_t j = 0; j < g_nodes.size(); j++) {
            NodeId v = g_nodes[j];
            // no action for colored nodes or cases where no colors where selected
            if (chosen_colors[v].size() == 0 || coloring[v] != 0) {
                continue;
            }

            auto selector = selectors[omp_get_thread_num()];

            VC difference = std::move(VC(chosen_colors[v]));
            VC temp(chosen_colors[v].size());

            for (NodeId u : g.out_neigh(v)) {
                // no action for already colored neighbours or neighbours with larger ID or ones that have chosen colors
                if (u >= v || chosen_colors[u].size() == 0 || coloring[u] != 0) {
                    continue;
                }
                auto temp_end = std::set_difference(difference.begin(), difference.end(), chosen_colors[u].begin(),
                                                    chosen_colors[u].end(), temp.begin());
                temp.resize(temp_end - temp.begin());
                std::swap(difference, temp);
            }
            if (difference.size() > 0) {
                new_color[v] = selector(difference);
                n_newly_colored++;
            }
        }
        made_progress = n_newly_colored > 0;
        n_colored += n_newly_colored;

        // update palettes
#pragma omp parallel for schedule(dynamic, 16)
        for (std::size_t j = 0; j < g_nodes.size(); j++) {
            NodeId v = g_nodes[j];
            // no action for colored nodes
            if (coloring[v] != 0) {
                continue;
            }

            for (NodeId u : g.out_neigh(v)) {
                // only neighbours that just got colored change the palette
                if (new_color[u] != 0) {
                    remove_sorted(palettes[v], new_color[u]);
                }
            }
        }

        // reset fields and actually set color
#pragma omp parallel for schedule(dynamic, 16)
        for (std::size_t j = 0; j < g_nodes.size(); j++) {
            NodeId v = g_nodes[j];
            // no action for nodes that were not colored in this round
            if (coloring[v] != 0 || new_color[v] == 0) {
                continue;
            }

            coloring[v] = new_color[v];
            new_color[v] = 0;
            chosen_colors[v].clear();
        }
    }

    return n_colored;
}

// g: the full graph
// coloring: the coloring to fill
// all_nodes: a vector containing a certain value for every node in g
// sparse_value: all nodes in all_nodes that are equal to sparse_value will be colored
template <class CGraph>
int coloring_elkin_subalgo_interface(const CGraph &g, VC &coloring, VI32 &all_nodes, int32_t sparse_value) {
    VN sparse_nodes(0);

#pragma omp parallel
    {
        VN sparse_local(0);

#pragma omp for schedule(dynamic, 16)
        for (std::size_t i = 0; i < all_nodes.size(); i++) {
            if (all_nodes[i] == sparse_value && coloring[i] == 0) {  // TODO
                sparse_local.push_back(i);
            }
        }

        if (sparse_local.size() > 0) {
#pragma omp critical
            push_back_move_all(sparse_local, sparse_nodes);
        }
    }
    std::sort(sparse_nodes.begin(), sparse_nodes.end());

    if (sparse_nodes.size() == 0) {
        return 0;
    }

    std::size_t n_colored = 0;
    return coloring_elkin(g, coloring, sparse_nodes, n_colored);
}

template <class CGraph>
int coloring_elkin_direct_interface(const CGraph &g, VC &coloring) {
    std::size_t n = g.num_nodes();
    VN g_nodes(n);
    std::generate(g_nodes.begin(), g_nodes.end(), [i = 0]() mutable { return i++; });

    std::size_t n_colored = 0;
    n_colored = coloring_elkin(g, coloring, g_nodes, n_colored);

    if (n_colored < n) {
        n_colored += coloring_barenboim_subalgo_interface(g, coloring);
    }

    return n_colored;
}

} // namespace GMS::Coloring

#endif  // COLORING_ELKIN_H_