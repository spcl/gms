#ifndef Kbit_Weighted_Neighbourhood_H
#define Kbit_Weighted_Neighbourhood_H

#include <inttypes.h>
//#include <xmmintrin.h>
//#include <ammintrin.h>
//#include <immintrin.h>
#include <gms/third_party/gapbs/benchmark.h>
#include "options.h"

typedef int32_t NodeId;
typedef int32_t WeightT;

// Copy of gapbs NodeWeight
// template <typename NodeId_, typename WeightT_>
struct Neighbour{
    NodeId id;
    WeightT weight;
    Neighbour(NodeId id, WeightT weight) : id(id), weight(weight) {}
};

using namespace std;

class Kbit_Weighted_Neighbourhood {
public:
    class iterator: public std::iterator<
                    std::input_iterator_tag, // iterator_category
                    Neighbour,                  // value_type
                    Neighbour,                  // difference_type
                    const NodeId*,           // pointer
                    Neighbour                // reference
                                  >{

        char* adjacencyArray;
        int8_t w; // number of bits for the edge weight
        int8_t kw; // number of bits for target id + edge weight
        int64_t mask_k; // mask for getting the edge target id
        int64_t mask_w; // mask for getting the edge weight
        int64_t exactBitOffset;
		#if SIMPLE_GAP_ENCODING
		NodeId current_vertex = 0;
		#endif


    public:
        WeightT weight;

        explicit iterator(int64_t exactBitOffset, void* adjacencyArray, int8_t k, int8_t w){
            this->adjacencyArray = (char*)adjacencyArray;
            this->w = w;
            this->kw = k + w;
            mask_k = ~((int64_t) -1 << k);
            mask_w = ~((int64_t) -1 << w);
			this->exactBitOffset = exactBitOffset;
        }

        iterator& operator++() {
            exactBitOffset += kw;
            return *this;
        }

        iterator operator++(int) {
            iterator retval = *this;
            exactBitOffset += kw;
            return retval;
        }

        bool operator==(iterator other) const {
            return exactBitOffset == other.exactBitOffset;
        }

        bool operator!=(iterator other) const {
            return exactBitOffset != other.exactBitOffset;
        }
        reference operator*() {
            char* address = adjacencyArray + (exactBitOffset >> 3);
            int64_t d = exactBitOffset & 7;
            int64_t value = ((int64_t*) (address))[0];
            value = value >> d;
			#if SIMPLE_GAP_ENCODING
				current_vertex += (value >> w) & mask_k;
            	return Neighbour(current_vertex, value & mask_w);
			#else
            	return Neighbour((value >> w) & mask_k, value & mask_w);
			#endif

			// alternative implementations which are worse:
            // return Neighbour(_bextr_u64(value, w, k), value & mask_w);
            // return Neighbour(_bextr_u64(value, d+w, k), _bextr_u64(value, d, w));
        }
    };

private:
    int64_t exactBitOffset;
    void* adjacencyArray;
    NodeId degree;
    int8_t k;
    int8_t w;
    int8_t kw;

public:
    Kbit_Weighted_Neighbourhood(NodeId degree, int64_t exactBitOffset, void* adjacencyArray, int8_t k, int8_t w){
        this->degree = degree;
        this->exactBitOffset = exactBitOffset;
        this->adjacencyArray = adjacencyArray;
        this->k = k;
        this->w = w;
        this->kw = k + w;
    }

    iterator begin() {
        return iterator(exactBitOffset, adjacencyArray, k, w);
    }

   iterator end() {
       return iterator(exactBitOffset + kw*degree, adjacencyArray, k, w);
   }

   /* Return the j'th neighbour of the neighbourhood */
   iterator get(int j){
       return iterator(exactBitOffset + kw*j, adjacencyArray, k, w);
   }

};

#endif // Kbit_Weighted_Neighbourhood_H
