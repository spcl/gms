#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/builder.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"
#include "coloring_common.h"

#include <vector>
#include <unordered_map>

// We need to know the cache line size at compile-time for alignas, but we can only determine it at runtime.
// TODO: What do? Just leave this hack with CACHE_LINE_SIZE = 64?
#define CACHE_LINE_SIZE 64

// Communication between threads using a lock-free queue

namespace GMS::Coloring::JonesV1 {

class message {
public:
    message *next;
    std::vector<NodeId> vertices;

    message() : next(nullptr), vertices() {}
};

class alignas(CACHE_LINE_SIZE) msg_queue {
private:
    message* head; // shared
    message* tail; // private

    msg_queue(message* sentinel) : head(sentinel), tail(sentinel) {}
public:
    msg_queue() : msg_queue(new message) {}
    ~msg_queue() {
        if (head != tail) {
            std::cout << "Tried to destroy a non-empty message queue" << std::endl;
            exit(EXIT_FAILURE);
        }
        delete head;
    }

    // Used by multiple threads to enqueue a message in a lock-free fashion
    void enqueue(message *new_head) {
        message *last_head;

        #pragma omp atomic capture
        {
            last_head = head;
            head = new_head;
        }

        #pragma omp atomic write
        last_head->next = new_head;
    }

    // Only called by the owner thread, spins until a message is available
    message* dequeue() {
        message **tail_next = &tail->next;
        message *next = nullptr;
        do {
            #pragma omp atomic read
            next = *tail_next;
        } while (next == nullptr);

        delete tail;
        tail = next;
        return next;
    }
};

void pack_and_send(std::vector<msg_queue> &msg_queues,
                   const std::vector<NodeId> &color_queue, // which nodes were colored
                   const std::vector<std::vector<NodeId>> &send_queue, // what nodes need to be notified of colorings
                   const int thread_count,
                   const int part_start,
                   const int64_t part_max_size) {

    std::vector<message*> messages(thread_count);
    for (NodeId v : color_queue) {
        int64_t v_node_idx = v - part_start;
        for (NodeId w : send_queue[v_node_idx]) {
            int tid = w / part_max_size;

            message *msg = messages[tid];
            if (!msg) {
                msg = new message;
                messages[tid] = msg;
            }
            msg->vertices.push_back(w);
        }
    }

    for (int tid = 0; tid < thread_count; ++tid) {
        message *msg = messages[tid];
        if (msg) {
            msg_queues[tid].enqueue(msg);
        }
    }
}

// Sequential coloring algorithms

template <class CGraph>
using seq_coloring_func = int32_t (*)(const CGraph& g, std::vector<int32_t> &coloring, const std::vector<NodeId> &color_queue, std::vector<NodeId>& order);

template <class CGraph>
int32_t pick_lowest_consistent_color(const CGraph& g, std::vector<int32_t> &coloring, const NodeId v) {
    // If all deg(v) neighbors have distinct colors 1..deg(v), then deg(v) + 1 will be a consistent color
    // Else, there will be a color i with 1 <= i <= deg(v) which was not selected for any neighbor
    const int32_t deg = g.out_degree(v);
    std::vector<bool> color_picked(deg + 1);

    for (NodeId w : g.out_neigh(v)) {
        int32_t w_color;
        #pragma omp atomic read
        w_color = coloring[w];
        color_picked[w_color] = true;
    }

    int32_t color;
    for (color = 1; color <= deg; ++color) {
        if (!color_picked[color]) break;
    }

    #pragma omp atomic write
    coloring[v] = color;

    return color;
}

template <class CGraph>
int32_t sequential_custom_order_coloring(const CGraph& g, std::vector<int32_t> &coloring, const std::vector<NodeId> &color_queue, std::vector<NodeId>& order) {
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

        int32_t color = pick_lowest_consistent_color(g, coloring, v);
        max_color = std::max(max_color, color);

        for (NodeId w : g.out_neigh(v)) {
            auto map_it = rev_map.find(w);
            if (map_it == rev_map.end()) continue;
            NodeId v = map_it->second;
            modified = true;
        }
    }

    return max_color;
}

// Actual parallel algorithm

template <class CGraph>
int32_t graph_coloring_jones(const CGraph& g, std::vector<int32_t> &coloring, std::vector<NodeId>& order) {
    const seq_coloring_func<CGraph> seq_color = &sequential_custom_order_coloring; // TODO: Determine which sequential function to use
    const int64_t n = g.num_nodes();

    // For each vertex i, rho[i] shall be a unique random number
    // TODO: Find an alternative to a random permutation
    std::vector<int64_t> rho;
    rho.reserve(n);
    for (int64_t i = 0; i < n; ++i) {
        rho.push_back(i);
    }
    random_shuffle(rho.begin(), rho.end());

    std::vector<msg_queue> msg_queues(omp_get_max_threads());
    int32_t num_colors = 0;

    // Entry to parallel section - implicit flush of (g, n, coloring, rho, msg_queues)
    #pragma omp parallel shared(g, coloring, rho, msg_queues) reduction(max: num_colors)
    {
        const int tcount = omp_get_num_threads();
        const int tid = omp_get_thread_num();
        const int64_t part_max_size = (n + (tcount - 1)) / tcount; // part_max_size = ceil(n / tcount);
        const int64_t part_start = std::min(n, tid * part_max_size);
        const int64_t part_end = std::min(n, part_start + part_max_size);
        const int64_t part_size = part_end - part_start; // Actual size of the i-th partition

        std::vector<NodeId> local_vertices;
        size_t shared_vertices_count = 0;

        std::vector<NodeId> color_queue; // Vertices to be colored
        std::vector<NodeId> n_wait(part_size);
        std::vector<std::vector<NodeId>> send_queue(part_size);

        for (NodeId v = part_start; v < part_end; ++v) {
            int64_t v_node_idx = v - part_start;
            bool is_local = true; // A vertex is local iff none of its neighbors are part of another partition

            for (NodeId w : g.out_neigh(v)) {
                if (part_start <= w && w < part_end) {
                    continue; // Skip local neighbors
                }

                is_local = false; // Has a shared edge, no longer a local vertex
                if (rho[w] > rho[v]) {
                    ++n_wait[v_node_idx];
                } else {
                    send_queue[v_node_idx].push_back(w);
                }
            }

            if (is_local) {
                local_vertices.push_back(v);
            } else {
                ++shared_vertices_count;
                if (n_wait[v_node_idx] == 0) {
                    // Vertex doesn't have to wait for any other vertices to be colored, so we can immediately color it
                    color_queue.push_back(v);
                }
            }
        }

        num_colors = seq_color(g, coloring, color_queue, order);
        size_t n_colored = color_queue.size();
        pack_and_send(msg_queues, color_queue, send_queue, tcount, part_start, part_max_size);
        color_queue.clear();

        msg_queue &in_queue = msg_queues[tid];
        while (n_colored < shared_vertices_count) {
            message *msg = in_queue.dequeue();

            for (NodeId v : msg->vertices) {
                int64_t v_node_idx = v - part_start;
                int waiting = --n_wait[v_node_idx];
                if (waiting == 0) {
                    color_queue.push_back(v);
                }
            }

            if (color_queue.empty()) {
                continue; // No vertices can be colored yet, wait for next message
            }

            int32_t cur_max_color = seq_color(g, coloring, color_queue, order);
            num_colors = std::max(num_colors, cur_max_color);
            n_colored += color_queue.size();
            pack_and_send(msg_queues, color_queue, send_queue, tcount, part_start, part_max_size);
            color_queue.clear();
        }

        int32_t cur_max_color = seq_color(g, coloring, local_vertices, order); // Color local vertices last
        num_colors = std::max(num_colors, cur_max_color);
    }

    return num_colors;
}

} // namespace GMS::Coloring::JonesV1