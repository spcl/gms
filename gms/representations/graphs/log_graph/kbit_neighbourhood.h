#ifndef Kbit_Neighbourhood_H
#define Kbit_Neighbourhood_H

#include <inttypes.h>
//#include <xmmintrin.h>
//#include <ammintrin.h>
//#include <immintrin.h>
#include <gms/third_party/gapbs/benchmark.h>
#include "options.h"

typedef int32_t NodeId;
typedef int32_t WeightT;

using namespace std;

// template<class T, class Tag = void>
class Kbit_Neighbourhood {
public:
    class iterator: public std::iterator<
                    std::input_iterator_tag, // iterator_category
                    NodeId,                  // value_type
                    NodeId,                  // difference_type
                    const NodeId*,           // pointer
                    NodeId                  // reference
                                  >{

        int32_t* adjacencyArray;
        // int64_t mask;
        int64_t exactBitOffset;
        int8_t k;
		#if SIMPLE_GAP_ENCODING
			NodeId current_vertex = 0;
		#endif


    public:
        explicit iterator(int64_t exactBitOffset, int32_t* adjacencyArray, int8_t k, int x){
            this->adjacencyArray = adjacencyArray;
            this->k = k;
			this->exactBitOffset = exactBitOffset + k*x;
            // mask = ~((int64_t) -1 << k);
        }

        iterator& operator++() {
            exactBitOffset += k;
            return *this;
        }

        iterator operator++(int) {
            iterator retval = *this;
            exactBitOffset += k;
            return retval;
        }

        bool operator==(iterator other) const {
            return exactBitOffset == other.exactBitOffset;
        }

        bool operator!=(iterator other) const {
            return exactBitOffset != other.exactBitOffset;
        }

        reference operator*() {
            int32_t* address = adjacencyArray + (exactBitOffset >> 5);
            int64_t d = exactBitOffset & 31;
			int64_t value = ((int64_t*) (address))[0];

			// TODO add back (cross platform compatibility issue - replace _bextr_u64)
			throw std::runtime_error("TODO Temporariliy disabled");

			/*
			#if SIMPLE_GAP_ENCODING
				current_vertex += _bextr_u64(value, d, k);
				return current_vertex;
			#else
				// cout << "bar" << endl;
            // return (value >> d) & mask;
            	return _bextr_u64(value, d, k);
			#endif
			*/
        }
    };

private:
    NodeId degree;
    int64_t exactBitOffset;
    int32_t* adjacencyArray;
    int8_t k;

public:
    // Kbit_Neighbourhood(NodeId degree, int64_t offset, int32_t* adjacencyArray, int8_t k){
    Kbit_Neighbourhood(NodeId degree, int64_t exactBitOffset, int32_t* adjacencyArray, int8_t k){
        this->degree = degree;
        this->exactBitOffset = exactBitOffset;
        this->adjacencyArray = adjacencyArray;
        this->k = k;
    }

    iterator begin() {
        return iterator(exactBitOffset, adjacencyArray, k, 0);
    }

   iterator end() {
       return iterator(exactBitOffset, adjacencyArray, k, degree);
   }

   /* Return the j'th neighbour of the neighbourhood */
   iterator get(int j){
       return iterator(exactBitOffset, adjacencyArray, k, j);
   }

};

#endif // Kbit_Neighbourhood_H
