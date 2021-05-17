
#ifndef GRAPHSETS_COMPRESSED_VARINT_WORD_BASED_H
#define GRAPHSETS_COMPRESSED_VARINT_WORD_BASED_H

#include "coders-utils/varint_utils.h"


class VarintWordBasedGraph {
public:
    class CompressedNeighbourhood {
    public:
        class iterator: public std::iterator<
                std::input_iterator_tag, // iterator_category
                NodeId,                  // value_type
                NodeId,                  // difference_type
                const NodeId*,           // pointer
                NodeId                  // reference
        >{

            unsigned char* pos;
            bool first_neigh;
            unsigned char sign;
            uint64_t neigh;
            uint64_t current_neigh;
            uint64_t v;

        public:
            explicit iterator(uint64_t v, unsigned char* pos, bool first_neigh, unsigned char sign, uint64_t current_neigh){
                this->v = v;
                this->pos = pos;
                this->current_neigh = current_neigh;

                uint64_t diff = 0;
                this->pos += fromVarint(this->pos, &diff);
                if (first_neigh) {
                    if (sign == NEXT_VAL_SMALLER) {
                        this->neigh = this->v - diff;
                    } else {
                        this->neigh = this->v + diff;
                    }
                }
            }


            iterator& operator++() {
                uint64_t diff = 0;
                this->pos += fromVarint(this->pos, &diff);
                this->current_neigh++;
                this->neigh = this->neigh + diff;
                return *this;
            }


            iterator operator++(int) {
                iterator retval = *this;
                uint64_t diff = 0;
                this->pos += fromVarint(this->pos, &diff);
                this->current_neigh++;
                this->neigh = this->neigh + diff;
                return retval;
            }


            bool operator==(iterator other) const {
                return this->current_neigh == other.current_neigh;
            }


            bool operator!=(iterator other) const {
                return this->current_neigh != other.current_neigh;
            }


            reference operator*() {
                return this->neigh;
            }
        };

    private:
        bool first_neigh;
        uint64_t nr_neighs = 0;
        unsigned char* pos;
        unsigned char sign;
        NodeId n;

    public:
        CompressedNeighbourhood(NodeId n, unsigned char* adj_data, uint64_t* offsets){
            this->n = n;
            this->nr_neighs = 0;
            this->pos = &adj_data[offsets[n] << 3];
            this->pos += fromVarint(pos, &nr_neighs);

            this->sign = *pos;
            this->pos++;
        }

        iterator begin() {
            return iterator(this->n, this->pos, true, this->sign, 0);
        }

        iterator end() {
            return iterator(this->n, this->pos, false, this->sign, nr_neighs);
        }
    };


public:

    uint64_t* new_offsets_in;
    uint64_t* new_offsets_out;
    uint64_t new_adj_data_size;
    unsigned char* new_adj_data_out;
    unsigned char* new_adj_data_in;

    VarintWordBasedGraph(int64_t num_nodes, int64_t num_edges, bool directed,
            uint64_t* new_offsets_out,
            uint64_t* new_offsets_in,
            unsigned char* new_adj_data_out,
            unsigned char* new_adj_data_in) {

        this->num_nodes_ = num_nodes;
        this->num_edges_ = num_edges;
        this->directed_ = directed;

        this->new_offsets_out = new_offsets_out;
        this->new_offsets_in = new_offsets_in;
        this->new_adj_data_out = new_adj_data_out;
        this->new_adj_data_in = new_adj_data_in;
    }

    VarintWordBasedGraph(const VarintWordBasedGraph &) = delete;
    VarintWordBasedGraph &operator=(const VarintWordBasedGraph &) = delete;

    VarintWordBasedGraph(VarintWordBasedGraph &&g) :
        new_offsets_out(g.new_offsets_out),
        new_offsets_in(g.new_offsets_in),
        new_adj_data_out(g.new_adj_data_out),
        new_adj_data_in(g.new_adj_data_in),
        directed_(g.directed_),
        num_nodes_(g.num_nodes_),
        num_edges_(g.num_edges_)
    {
        g.new_offsets_out = nullptr;
        g.new_offsets_in = nullptr;
        g.new_adj_data_out = nullptr;
        g.new_adj_data_in = nullptr;
    }

    ~VarintWordBasedGraph() {
        if (new_offsets_out) delete[] new_offsets_out;
        if (new_offsets_in) delete[] new_offsets_in;
        if (new_adj_data_out) delete[] new_adj_data_out;
        if (new_adj_data_in) delete[] new_adj_data_in;
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

    int64_t out_degree(NodeId v) const {
        uint64_t nr_neighs = 0;
        unsigned char* pos = &new_adj_data_out[new_offsets_out[v] << 3];
        fromVarint(pos, &nr_neighs);

        return (int64_t)nr_neighs;
    }

    int64_t in_degree(NodeId v) const {
        uint64_t nr_neighs = 0;
        unsigned char* pos = &new_adj_data_in[new_offsets_in[v] << 3];
        fromVarint(pos, &nr_neighs);

        return (int64_t)nr_neighs;
    }

    CompressedNeighbourhood out_neigh(NodeId n) const {
        return CompressedNeighbourhood(n, new_adj_data_out, new_offsets_out);
    }

    CompressedNeighbourhood in_neigh(NodeId n) const {
        return CompressedNeighbourhood(n, new_adj_data_in, new_offsets_in);
    }

    void PrintStats() const {
        std::cout << "Varint byte-based graph has " << num_nodes() << " nodes and "
                  << num_edges() << " ";
        if (!directed()){
            std::cout << "un";
        }
        std::cout << "directed edges for degree: ";
        std::cout << (num_edges()/num_nodes()) << std::endl;
    }

    void PrintTopology() const {
    }

    Range<NodeId> vertices() const {
        return Range<NodeId>(num_nodes());
    }



private:
    bool directed_;
    int64_t num_nodes_;
    int64_t num_edges_;
};


#endif //GRAPHSETS_COMPRESSED_CSR_GRAPH_H
