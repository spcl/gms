#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/builder.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"
#include "coloring_common.h"

#include <vector>
#include <unordered_map>
#include <random>

// We need to know the cache line size at compile-time for alignas, but we can only determine it at runtime.
// TODO: What do? Just leave this hack with CACHE_LINE_SIZE = 64?
#define CACHE_LINE_SIZE 64


// Communication between threads using a lock-free queue

namespace GMS::Coloring::JonesV4 {

class alignas(CACHE_LINE_SIZE) ready_queue {
private:
    NodeId* data; // shared
    size_t write_pos; // shared
    size_t read_pos; // private

public:
    ready_queue() : data(nullptr), write_pos(0), read_pos(0) {}
    ~ready_queue() {
        if (write_pos != read_pos) {
            std::cout << "Tried to destroy a non-empty ready_queue" << std::endl;
            exit(EXIT_FAILURE);
        }
        delete[] data;
    }

    void init(size_t max_size) {
        if (data != nullptr) {
            std::cout << "Tried to re-init a ready_queue" << std::endl;
            exit(EXIT_FAILURE);
        }
        data = new NodeId[max_size](); // Zero-initialized
    }

    // Used by multiple threads to notify a thread that one of its vertices is now ready
    void enqueue(NodeId ready_vertex) {
        size_t my_write_pos;
        #pragma omp atomic capture
        my_write_pos = write_pos++;
        #pragma omp atomic write
        data[my_write_pos] = (ready_vertex + 1);
    }

    // Only called by the owner thread, spins until a ready vertex is available
    void dequeue(std::vector<NodeId> &color_queue) {
        // Wait until at least one ready vertex is available
        size_t cur_write_pos;
        do {
            #pragma omp atomic read
            cur_write_pos = write_pos;
        } while (cur_write_pos == read_pos);

        int64_t ready_vertex;
        for (; read_pos < cur_write_pos; ++read_pos) {
            // Wait until a thread writes to data[read_pos], i.e. data[read_pos] != 0
            do {
                #pragma omp atomic read
                ready_vertex = data[read_pos];
            } while (ready_vertex == 0);

            color_queue.push_back(ready_vertex - 1);
        }
    }
};

// Book-keeping of which vertices to notify if a vertex gets colored

class node_queue {
private:
    const NodeId* beginPtr;
    const NodeId* endPtr;
public:
    node_queue(const NodeId* begin, const NodeId* end) : beginPtr(begin), endPtr(end) {}

    const NodeId* begin() const {
        return beginPtr;
    }
    const NodeId* end() const {
        return endPtr;
    }
};

class node_queue_list {
private:
    int64_t part_start;
    size_t cur_node_idx;
    size_t cur_offset_idx;
    NodeId *nodes;
    size_t *node_offsets;

public:
    node_queue_list(int64_t part_start, int64_t part_end, size_t max_size)
        : part_start(part_start),
          cur_node_idx(0),
          cur_offset_idx(1),
          nodes(new NodeId[max_size]),
          node_offsets(new size_t[part_end - part_start + 1])
    {
        node_offsets[0] = 0;
    }
    ~node_queue_list() {
        delete[] nodes;
        delete[] node_offsets;
    }

    void insert(NodeId node) {
        nodes[cur_node_idx++] = node;
    }

    void next_node() {
        node_offsets[cur_offset_idx++] = cur_node_idx;
    }

    const node_queue operator[](const NodeId v) const {
        const int64_t v_idx = v - part_start;
        size_t begin = node_offsets[v_idx];
        size_t end = node_offsets[v_idx+1];
        return node_queue(&nodes[begin], &nodes[end]);
    }
};

void notify_threads(const node_queue send_queue, std::vector<int32_t> &n_wait,
                    std::vector<ready_queue> &ready_queues, const int64_t part_max_size) {

    for (NodeId w : send_queue) {
        int32_t num_waiting;
        #pragma omp atomic capture
        num_waiting = --n_wait[w];
        if (num_waiting == 0) {
            // w now isn't waiting for any other vertices anymore, so tell the w's thread that w is ready
            size_t w_thread_id = w / part_max_size;
            ready_queues[w_thread_id].enqueue(w);
        }
    }
}

// Sequential coloring algorithms

template <class CGraph>
int32_t pick_lowest_consistent_color(const CGraph& g, std::vector<int32_t> &coloring,
                                     const NodeId v, std::vector<bool> &color_palette) {
    // If all deg(v) neighbors have distinct colors 1..deg(v), then deg(v) + 1 will be a consistent color
    // Else, there will be a color i with 1 <= i <= deg(v) which was not selected for any neighbor
    const int32_t deg = g.out_degree(v);
    for (NodeId w : g.out_neigh(v)) {
        int32_t w_color;
        #pragma omp atomic read
        w_color = coloring[w];
        if (w_color <= deg) {
            color_palette[w_color] = true;
        }
    }

    int32_t color;
    for (color = 1; color <= deg; ++color) {
        if (!color_palette[color]) break;
    }

    #pragma omp atomic write
    coloring[v] = color;

    // Reset the color pallette to false - only [0..deg] were used
    std::fill(color_palette.begin(), color_palette.begin() + (deg + 1), false);

    return color;
}

template <class CGraph>
using seq_coloring_func = int32_t (*)(const CGraph& g, std::vector<int32_t> &coloring, const std::vector<NodeId> &color_queue,
                                      std::vector<int32_t> &n_wait, const node_queue_list &send_queues,
                                      std::vector<ready_queue> &ready_queues, const int64_t part_max_size,
                                      std::vector<bool> &color_palette, std::vector<NodeId>& order);

// int32_t sequential_coloring_unordered(const CGraph& g, std::vector<int32_t> &coloring, const std::vector<NodeId> &color_queue,
//                                       std::vector<int32_t> &n_wait, const node_queue_list &send_queues,
//                                       std::vector<ready_queue> &ready_queues, const int64_t part_max_size,
//                                       std::vector<bool> &color_palette) {
//     int32_t max_color = 0;
//     for (NodeId v : color_queue) {
//         int32_t color = pick_lowest_consistent_color(g, coloring, v, color_palette);
//         max_color = std::max(max_color, color);
//         notify_threads(send_queues[v], n_wait, ready_queues, part_max_size);
//     }
//     return max_color;
// }

// int32_t sequential_coloring_ldo(const CGraph& g, std::vector<int32_t> &coloring, const std::vector<NodeId> &color_queue,
//                                 std::vector<int32_t> &n_wait, const node_queue_list &send_queues,
//                                 std::vector<ready_queue> &ready_queues, const int64_t part_max_size,
//                                 std::vector<bool> &color_palette, std::vector<NodeId>& order) {
//     struct idx_and_degree {
//         size_t index;
//         int64_t degree;

//         idx_and_degree(size_t i, int64_t d) : index(i), degree(d) {}
//         bool operator <(const idx_and_degree &other) {
//             return degree > other.degree; // Sort descending by degree
//         }
//     };

//     int32_t max_color = 0;
//     size_t n_to_color = color_queue.size();
//     std::vector<idx_and_degree> work_list;
//     work_list.reserve(n_to_color);

//     for (size_t i = 0; i < n_to_color; ++i) {
//         NodeId v = color_queue[i];
//         work_list.emplace_back(i, g.out_degree(v));
//     }

//     std::sort(work_list.begin(), work_list.end());

//     for (idx_and_degree item : work_list) {
//         NodeId v = color_queue[item.index];
//         int32_t color = pick_lowest_consistent_color(g, coloring, v, color_palette);
//         max_color = std::max(max_color, color);
//         notify_threads(send_queues[v], n_wait, ready_queues, part_max_size);
//     }

//     return max_color;
// }

template <class CGraph>
int32_t sequential_custom_order_coloring(const CGraph& g, std::vector<int32_t> &coloring, const std::vector<NodeId> &color_queue,
                                         std::vector<int32_t> &n_wait, const node_queue_list &send_queues,
                                         std::vector<ready_queue> &ready_queues, const int64_t part_max_size,
                                         std::vector<bool> &color_palette, std::vector<NodeId>& order) {
    int32_t max_color = 0;

    size_t n_to_color = color_queue.size();
    std::vector<size_t> max_deg_heap(n_to_color);
    std::unordered_map<NodeId, size_t> rev_map;

    for (size_t i = 0; i < n_to_color; ++i) {
        NodeId v = color_queue[i];
        rev_map[v] = i;
        max_deg_heap[i] = i;
    }

    bool modified = true;
    for (size_t i = 0; i < n_to_color; ++i) {
        if (modified) {
            std::make_heap(max_deg_heap.begin(), max_deg_heap.end(), [&](const size_t a, const size_t b) -> bool {
                return order[a] < order[b];
            });
            modified = false;
        }

        NodeId v = color_queue[max_deg_heap[0]];
        std::pop_heap(max_deg_heap.begin(), max_deg_heap.end());
        max_deg_heap.pop_back();

        int32_t color = pick_lowest_consistent_color(g, coloring, v, color_palette);
        max_color = std::max(max_color, color);
        notify_threads(send_queues[v], n_wait, ready_queues, part_max_size);

        for (NodeId w : g.out_neigh(v)) {
            auto map_it = rev_map.find(w);
            if (map_it == rev_map.end()) continue;
            NodeId v = map_it->second;
            modified = true;
        }
    }

    return max_color;
}

// Graph partitioning

uint64_t rho(const uint32_t seed, uint64_t v) {
    const uint64_t rnd_prime_64 = 0xE57EACE69B044FE7ULL;
    v = (v * rnd_prime_64) + seed;
    v = (v >> 17) | (v << 47);
    v = (v + seed) * rnd_prime_64;
    return v;
}

template <class CGraph>
size_t partition_graph(const CGraph &g, const int64_t part_start, const int64_t part_end, const uint32_t rho_seed,
                       std::vector<int32_t> &n_wait, node_queue_list &send_queue,
                       std::vector<NodeId> &color_queue, std::vector<NodeId> &local_vertices) {
    int64_t max_degree = 0;

    for (NodeId v = part_start; v < part_end; ++v) {
        bool is_local = true; // A vertex is local iff none of its neighbors are part of another partition
        int32_t n_wait_v = 0;
        uint64_t rho_v = rho(rho_seed, (uint64_t) v);

        for (NodeId w : g.out_neigh(v)) {
            if (part_start <= w && w < part_end) {
                continue; // Skip local neighbors
            }

            is_local = false; // Has a shared edge, no longer a local vertex

            uint64_t rho_w = rho(rho_seed, (uint64_t) w);
            if (rho_w > rho_v) {
                ++n_wait_v;
            } else {
                send_queue.insert(w);
            }
        }

        if (is_local) {
            local_vertices.push_back(v);
        } else if (n_wait_v == 0) {
            // Shared vertex doesn't have to wait for any other vertices to be colored, so we can immediately color it
            color_queue.push_back(v);
        }

        max_degree = std::max(max_degree, g.out_degree(v));
        n_wait[v] = n_wait_v;
        send_queue.next_node();
    }

    return max_degree;
}

// Actual parallel algorithm

template <class CGraph>
int32_t graph_coloring_jones(const CGraph &g, std::vector<int32_t> &coloring, std::vector<NodeId>& order) {
//    DetailTimer timer;

    const seq_coloring_func<CGraph> seq_color = &sequential_custom_order_coloring;
    const int64_t n = g.num_nodes();

    std::vector<ready_queue> ready_queues(omp_get_max_threads());
    std::vector<int32_t> n_wait(n);
    int32_t num_colors = 0;
    size_t shared_vertices_count = 0;

    std::random_device rd;
    const uint32_t rho_seed = rd();

    std::cout << omp_get_max_threads() << " " << omp_get_num_threads() << std::endl;

//    timer.endPhase("init");

#pragma omp parallel shared(g, coloring, ready_queues, n_wait) reduction(max: num_colors) reduction(+: shared_vertices_count)
    {
        const int tcount = omp_get_num_threads();
        const int tid = omp_get_thread_num();
        const int64_t part_max_size = (n + (tcount - 1)) / tcount; // part_max_size = ceil(n / tcount);
        const int64_t part_start = std::min(n, tid * part_max_size);
        const int64_t part_end = std::min(n, part_start + part_max_size);
        const int64_t part_size = part_end - part_start;

        std::vector<NodeId> color_queue; color_queue.reserve(part_size);
        std::vector<NodeId> local_vertices; local_vertices.reserve(part_size);
        const size_t num_partition_neighbors = std::distance(g.out_neigh(part_start).begin(), g.out_neigh(part_end - 1).end());
        node_queue_list send_queues(part_start, part_end, num_partition_neighbors);

        size_t max_degree = partition_graph(g, part_start, part_end, rho_seed, n_wait, send_queues, color_queue, local_vertices);
        shared_vertices_count = part_size - local_vertices.size();

        std::vector<bool> color_palette(max_degree + 1, false);
        ready_queue &in_queue = ready_queues[tid];
        in_queue.init(shared_vertices_count);
#pragma omp barrier
        // All n_wait and ready_queues are now initialized

#pragma omp master
        {
//            timer.endPhase("par_partition");
        }

        num_colors = seq_color(g, coloring, color_queue, n_wait, send_queues, ready_queues, part_max_size, color_palette, order);
        size_t n_colored = color_queue.size();
        color_queue.clear();

        while (n_colored < shared_vertices_count) {
            in_queue.dequeue(color_queue);
            int32_t cur_max_color = seq_color(g, coloring, color_queue, n_wait, send_queues, ready_queues, part_max_size, color_palette, order);
            num_colors = std::max(num_colors, cur_max_color);
            n_colored += color_queue.size();
            color_queue.clear();
        }

        // Color local vertices last
        int32_t cur_max_color = seq_color(g, coloring, local_vertices, n_wait, send_queues, ready_queues, part_max_size, color_palette, order);
        num_colors = std::max(num_colors, cur_max_color);
    }

//    timer.endPhase("par_coloring");
//    timer.print();

    double local_ratio = 1.0 - (double) shared_vertices_count / n;
    std::cout << "Local vertex ratio: " << local_ratio << std::endl;

    return num_colors;
}

} // namespace GMS::Coloring::JonesV4