// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef GRAPH_H_
#define GRAPH_H_

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <iostream>
#include <type_traits>

#include "immintrin.h"
#include "pvector.h"
#include "util.h"

#include <gms/common/types.h>

/*
GAP Benchmark Suite
Class:  CSRGraph
Author: Scott Beamer

Simple container for graph in CSR format
 - Intended to be constructed by a Builder
 - To make weighted, set DestID_ template type to NodeWeight
 - MakeInverse parameter controls whether graph stores its inverse
*/

// Used to hold node & weight, with another node it makes a weighted edge
template <typename NodeID_, typename WeightT_>
struct NodeWeight {
    NodeID_ v;
    WeightT_ w;
    NodeWeight() {}
    NodeWeight(NodeID_ v) : v(v), w(1) {}
    NodeWeight(NodeID_ v, WeightT_ w) : v(v), w(w) {}

    bool operator< (const NodeWeight& rhs) const {
        return v == rhs.v ? w < rhs.w : v < rhs.v;
    }

    // doesn't check WeightT_s, needed to remove duplicate edges
    bool operator== (const NodeWeight& rhs) const {
        return v == rhs.v;
    }

    // doesn't check WeightT_s, needed to remove self edges
    bool operator== (const NodeID_& rhs) const {
        return v == rhs;
    }

    operator NodeID_() {
        return v;
    }
};

template <typename NodeID_, typename WeightT_>
std::ostream& operator<<(std::ostream& os,
                         const NodeWeight<NodeID_, WeightT_>& nw) {
    os << nw.v << " " << nw.w;
    return os;
}

template <typename NodeID_, typename WeightT_>
std::istream& operator>>(std::istream& is, NodeWeight<NodeID_, WeightT_>& nw) {
    is >> nw.v >> nw.w;
    return is;
}



// Syntatic sugar for an edge
template <typename SrcT, typename DstT = SrcT>
struct EdgePair {
    SrcT u;
    DstT v;

    EdgePair() {}

    EdgePair(SrcT u, DstT v) : u(u), v(v) {}
};
template <typename SrcT, typename DstT = SrcT>
std::ostream& operator<<(std::ostream& os, const EdgePair<SrcT, DstT>& ep) {
    os << "u=" << ep.u << "v=" << ep.v;
    return os;
}
// SG = serialized graph, these types are for writing graph to file
typedef int32_t SGID;
typedef EdgePair<SGID> SGEdge;
typedef int64_t SGOffset;

template <class NodeID_, class DestID_ = NodeID_, bool MakeInverse = true>
class CSRGraphBase {
    // Used for *non-negative* offsets within a neighborhood
    typedef std::make_unsigned<std::ptrdiff_t>::type OffsetT;

    // Used to access neighbors of vertex, basically sugar for iterators
    class Neighborhood {
        NodeID_ n_;
        DestID_** g_index_;
        OffsetT start_offset_;
    public:
        Neighborhood(NodeID_ n, DestID_** g_index, OffsetT start_offset) :
                n_(n), g_index_(g_index), start_offset_(0) {
            OffsetT max_offset = end() - begin();
            start_offset_ = std::min(start_offset, max_offset);
        }
        typedef DestID_* iterator;
        iterator begin() { return g_index_[n_] + start_offset_; }
        iterator end()   { return g_index_[n_+1]; }

        typedef const DestID_* const_iterator;
        const_iterator begin() const { return g_index_[n_] + start_offset_; }
        const_iterator end() const   { return g_index_[n_+1]; }
    };

    void ReleaseResources() {
        // ============================================================================
        // Added by Jakub Golinowski:
        // It is to account for the fact that in case of padding the align_malloc function is used isntead of operator new.
        if(alignment_>1){
            if (out_index_ != nullptr)
            {
                free(out_index_);
                out_index_ = nullptr;
            }
            if (out_neighbors_ != nullptr)
            {
                free(out_neighbors_);
                out_neighbors_ = nullptr;
            }
            if (directed_) {
                if (in_index_ != nullptr)
                {
                    free(in_index_);
                    in_index_ = nullptr;
                }
                if (in_neighbors_ != nullptr)
                {
                    free(in_neighbors_);
                    in_neighbors_ = nullptr;
                }
            }
        } else {
            // ============================================================================
            if (out_index_ != nullptr)
            {
                delete[] out_index_;
                out_index_ = nullptr;
            }
            if (out_neighbors_ != nullptr)
            {
                delete[] out_neighbors_;
                out_neighbors_ = nullptr;
            }
            if (directed_) {
                if (in_index_ != nullptr)
                {
                    delete[] in_index_;
                    in_index_ = nullptr;
                }
                if (in_neighbors_ != nullptr)
                {
                    delete[] in_neighbors_;
                    in_neighbors_ = nullptr;
                }
            }
        }
    }


public:
    // added by Yannick Schaffner to access Neighborhood-class
    typedef Neighborhood Neighborhood_t;

    CSRGraphBase() : directed_(false), num_nodes_(-1), num_edges_(-1),
                 out_index_(nullptr), out_neighbors_(nullptr),
                 in_index_(nullptr), in_neighbors_(nullptr), alignment_(1), index_guarded_1_based_(false) {}

    CSRGraphBase(int64_t num_nodes, DestID_** index, DestID_* neighs, int64_t alignment=1, bool index_guarded_1_based=false) :
            directed_(false), num_nodes_(num_nodes),
            out_index_(index), out_neighbors_(neighs),
            in_index_(index), in_neighbors_(neighs), alignment_(alignment), index_guarded_1_based_(index_guarded_1_based) {
        if(index_guarded_1_based) num_edges_ = (out_index_[num_nodes_+1] - out_index_[1]) / 2;
        else num_edges_ = (out_index_[num_nodes_] - out_index_[0]) / 2;
    }

    CSRGraphBase(int64_t num_nodes, DestID_** out_index, DestID_* out_neighs,
             DestID_** in_index, DestID_* in_neighs, int64_t alignment=1) :
            directed_(true), num_nodes_(num_nodes),
            out_index_(out_index), out_neighbors_(out_neighs),
            in_index_(in_index), in_neighbors_(in_neighs), alignment_(alignment), index_guarded_1_based_(false){
        num_edges_ = out_index_[num_nodes_] - out_index_[0];
    }

    CSRGraphBase(CSRGraphBase&& other) : directed_(other.directed_),
                                 num_nodes_(other.num_nodes_), num_edges_(other.num_edges_), out_index_(other.out_index_),
                                 out_neighbors_(other.out_neighbors_), in_index_(other.in_index_), in_neighbors_(other.in_neighbors_),
                                 alignment_(other.alignment_), index_guarded_1_based_(other.index_guarded_1_based_) {
        other.num_edges_ = -1;
        other.num_nodes_ = -1;
        other.out_index_ = nullptr;
        other.out_neighbors_ = nullptr;
        other.in_index_ = nullptr;
        other.in_neighbors_ = nullptr;
        other.alignment_ = 1;
        other.index_guarded_1_based_=false;
    }

    ~CSRGraphBase() {
        ReleaseResources();
    }

    CSRGraphBase& operator=(CSRGraphBase&& other) {
        if (this != &other) {
            ReleaseResources();
            directed_ = other.directed_;
            num_edges_ = other.num_edges_;
            num_nodes_ = other.num_nodes_;
            out_index_ = other.out_index_;
            out_neighbors_ = other.out_neighbors_;
            in_index_ = other.in_index_;
            in_neighbors_ = other.in_neighbors_;
            index_guarded_1_based_ = other.index_guarded_1_based_;
            other.num_edges_ = -1;
            other.num_nodes_ = -1;
            other.out_index_ = nullptr;
            other.out_neighbors_ = nullptr;
            other.in_index_ = nullptr;
            other.in_neighbors_ = nullptr;
        }
        return *this;
    }

    bool directed() const {
        return directed_;
    }

    int64_t num_nodes() const {
        return num_nodes_;
    }

    int64_t num_edges() const {
        return num_edges_;
    }

    int64_t num_edges_directed() const {
        return directed_ ? num_edges_ : 2*num_edges_;
    }

    int64_t out_degree(NodeID_ v) const {
        return out_index_[v+1] - out_index_[v];
    }

    int64_t in_degree(NodeID_ v) const {
        static_assert(MakeInverse, "Graph inversion disabled but reading inverse");
        return in_index_[v+1] - in_index_[v];
    }

    Neighborhood out_neigh(NodeID_ n, OffsetT start_offset = 0) const {
        return Neighborhood(n, out_index_, start_offset);
    }

    Neighborhood in_neigh(NodeID_ n, OffsetT start_offset = 0) const {
        static_assert(MakeInverse, "Graph inversion disabled but reading inverse");
        return Neighborhood(n, in_index_, start_offset);
    }

    void PrintStats() const {
        std::cout << "Graph has " << num_nodes_ << " nodes and "
                  << num_edges_ << " ";
        if (!directed_)
            std::cout << "un";
        std::cout << "directed edges for degree: ";
        std::cout << num_edges_/num_nodes_ << std::endl;
    }

    void PrintTopology() const {
        if(index_guarded_1_based_){
            for (NodeID_ i=1; i < num_nodes_+1; i++) {
                std::cout << i << ": ";
                for (DestID_ j : out_neigh(i)) {
                    std::cout << j << " ";
                }
                std::cout << std::endl;
            }
        } else {
            for (NodeID_ i = 0; i < num_nodes_; i++) {
                std::cout << i << ": ";
                for (DestID_ j : out_neigh(i)) {
                    std::cout << j << " ";
                }
                std::cout << std::endl;
            }
        }
    }

    static DestID_** GenIndex(const pvector<SGOffset> &offsets, DestID_* neighs, bool add_guard=false) {
        // ============================================================================
        // Added by Jakub Golinowski:
        if(add_guard){
            NodeID_ length = offsets.size()+1;
            DestID_** index = new DestID_*[length];
            index[0] = kBeamerIndexGuardValue;
#pragma omp parallel for
            for (NodeID_ n=1; n < length; n++)
                index[n] = neighs + offsets[n-1];
            return index;
            // ============================================================================
        } else {
            NodeID_ length = offsets.size();
            DestID_ **index = new DestID_ *[length];
#pragma omp parallel for
            for (NodeID_ n = 0; n < length; n++)
                index[n] = neighs + offsets[n];
            return index;
        }
    }

    // ============================================================================
    // Added by Jakub Golinowski:
    static DestID_** GenIndexAligned(const pvector<SGOffset> &offsets, DestID_* neighs) {
        NodeID_ length = offsets.size();
        DestID_** index;
        align_malloc((void**)&index, sizeof(__m256i), sizeof(DestID_*)*length);
#pragma omp parallel for
        for (NodeID_ n=0; n < length; n++)
            index[n] = neighs + offsets[n];
        return index;
    }

    int64_t alignment() const { return alignment_; }
    // ============================================================================

    pvector<SGOffset> VertexOffsets(bool in_graph = false) const {
        pvector<SGOffset> offsets(num_nodes_+1);
        for (NodeID_ n=0; n < num_nodes_+1; n++)
            if (in_graph)
                offsets[n] = in_index_[n] - in_index_[0];
            else
                offsets[n] = out_index_[n] - out_index_[0];
        return offsets;
    }

    Range<NodeID_> vertices() const {
        return Range<NodeID_>(num_nodes());
    }

    // ============================================================================
    // Added by Jakub Golinowski:
    bool isEmpty() const {
        return (num_nodes_==-1 && num_edges_==-1 && out_index_==nullptr && out_neighbors_==nullptr
                && in_index_==nullptr && in_neighbors_==nullptr);
    }
    // ============================================================================
//private:
    bool directed_;
    int64_t num_nodes_;
    int64_t num_edges_;
    DestID_** out_index_;
    DestID_*  out_neighbors_;
    DestID_** in_index_;
    DestID_*  in_neighbors_;
    // ============================================================================
    // Added by Jakub Golinowski:
    int64_t alignment_;
    bool index_guarded_1_based_;

    static constexpr DestID_* kBeamerIndexGuardValue=0;
    // ============================================================================
};

typedef CSRGraphBase<NodeId> CSRGraph;

#endif  // GRAPH_H_
