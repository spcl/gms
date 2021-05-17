#ifndef Bit_Tree_Neighbourhood_H
#define Bit_Tree_Neighbourhood_H

#include <inttypes.h>
#include <xmmintrin.h>
#include <ammintrin.h>
#include <immintrin.h>
#include <gms/third_party/gapbs/benchmark.h>
#include "options.h"

typedef int32_t NodeId;
typedef int32_t WeightT;

using namespace std;

// int a = 0;
// int b = 0;
// int c = 0;
// int d = 0;

// template<class T, class Tag = void>
class Bit_Tree_Neighbourhood {
public:
    class iterator: public std::iterator<
                    std::input_iterator_tag, // iterator_category
                    NodeId,                  // value_type
                    NodeId,                  // difference_type
                    const NodeId*,           // pointer
                    NodeId                  // reference
                                  >{

		My_Bitmap* tree;
        int64_t position;
		// stack faster than heap:  http://stackoverflow.com/a/24057855
		// NodeId* stack;
		NodeId stack[32];
		int8_t stack_top;
        int8_t k;
		bool interval_mode;
		NodeId current_vertex;
		NodeId interval_end;

    public:
        explicit iterator(My_Bitmap* tree){
			this->position = tree->get_size();
		}

        explicit iterator(My_Bitmap* tree, int8_t k){
            this->tree= tree;
            this->k = k;
			this->position = 0;
			// stack = (NodeId*) malloc(k*sizeof(NodeId));
			stack_top = 0;
			stack[stack_top ++] = 1<<(k-1);

			this->interval_mode = false;
			this->interval_end = -1;
        }

        iterator& operator++() {
            return *this;
        }

        iterator operator++(int) {
            iterator retval = *this;
            return retval;
        }

        bool operator==(iterator other) const {
            return position == other.position;
        }

        bool operator!=(iterator other) const {
            return position != other.position;
        }

        reference operator*() {
			if(interval_mode){
				current_vertex ++;
				if(current_vertex == interval_end){
					interval_mode = false;
					position += 2;
				}
				return current_vertex;
			}

			while(true){
				int code = tree->get_2bits(position);
				int output = stack[--stack_top];
				switch(code){
					case 0:  // very low probability
						interval_mode = true;
						current_vertex = _blsr_u32(output);
						interval_end = output | _blsmsk_u32(output);
						return current_vertex;
					case 1: // 3/7 probability
						position += 2;
						if (output & 1) {
							return output;
						}
						else{
							uint32_t selector = _blsi_u32(output) >> 1;
							stack[stack_top ++] = output | selector;
						}
					break;
					case 2: // 3/7 probability
						position += 2;
						if (output & 1) {
							return _blsr_u32(output);
						}
						else{
							uint32_t selector = _blsi_u32(output) >> 1;
							stack[stack_top ++] = _blsr_u32(output) | selector;
						}
					break;
					case 3: // 1/7 probability
					{
						position += 2;
						uint32_t selector = _blsi_u32(output) >> 1;
						stack[stack_top] = output | selector;
						stack[stack_top + 1] = _blsr_u32(output) | selector;
						stack_top += 2;
					}
					break;
				}
			}
        }
    };

private:
	My_Bitmap* tree;
    int8_t k;

public	:
    Bit_Tree_Neighbourhood(My_Bitmap* tree, int8_t k){
		this->tree = tree;
        this->k = k;
    }

    iterator begin() {
        return iterator(tree, k);
    }

   iterator end() {
       return iterator(tree);
   }

};

#endif // Bit_Tree_Neighbourhood_H
