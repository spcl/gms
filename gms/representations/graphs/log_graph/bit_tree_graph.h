#ifndef Bit_Tree_Graph_H
#define Bit_Tree_Graph_H

#include <iostream>
#include <mmintrin.h>
#include <inttypes.h>
#include <mmintrin.h>
#include <immintrin.h>
#include "my_bitmap.h"

#include "kbit_neighbourhood.h"
#include <gms/third_party/gapbs/util.h>
#include "bit_tree_neighbourhood.h"

#define BYTE 8

using namespace std;

/* contains either the bit offset in the adjacency array or a pointer to the
	bit tree depending on whether the corresponding vertex has been encoded
	with the bit tree encoding */
union Offset_Or_Address{
	int64_t offset;
	My_Bitmap* tree;
};

struct Offset_Array_Entry{
	int32_t degree;
	int8_t bitlength;
	int8_t encoding; // 0 if k_bit, 1 if bit_tree
	Offset_Or_Address offset_or_tree;
};

class Bit_Tree_Graph {
	private:
		int64_t n; // number of nodes
		int64_t m; // number of edges
		bool isDirected; // whether the graph is directed
        // int64_t* O;
        Offset_Array_Entry* O;
		int64_t* offsetArray; // contains edge offsets (only needed for bc)
        int32_t* adjacencyArray;
		const int64_t mask_5 = ~((int64_t) -1 << 5); // selects first 5 bits
		const int64_t mask_59 = ~((int64_t) -1 << 59); // selectes first 59 bits

	public:
        /* Creates an empty graph */
        Bit_Tree_Graph(bool directed){
		    isDirected = directed;
		}

        /* Create a graph with n vertices and m edges. Each vertex represented with k bits */
        Bit_Tree_Graph(int64_t nVertices, int64_t nEdges,
			bool directed, int32_t* adjacencyArray, Offset_Array_Entry* O){
		    n = nVertices;
		    m = nEdges;
			this->O = O;
			this->adjacencyArray = adjacencyArray;
		    isDirected = directed;
		}

		// Destructor
		~Bit_Tree_Graph() {
			for(NodeId v=0; v < n; v ++) {
				if(encoding(v)){
					delete O[v].offset_or_tree.tree;
				}
			}
			free(O);
			free(adjacencyArray);
		}

		/* Returns the number of vertices */
		int64_t num_nodes() const {
			return n;
		}

		/* Returns the number of edges */
		int64_t num_edges() const {
			return m;
		}

		/* Returns the number of edges, counting every edge twice if the graph is undirected */
		int64_t num_edges_directed() const {
			return isDirected ? m : 2*m;
		}

		/* Returns whether the graph is directed */
		bool directed() const {
			return isDirected;
		}

        /* Returns whether the vertices u and v are connected.
			Performs binary search through all neighbours of the vertex
			with less neighbours
		*/
        bool connected(NodeId u, NodeId v) const{
			if(out_degree(v) < out_degree(u)){
				NodeId t = u;
				u = v;
				v = t;
			}
			Kbit_Neighbourhood neighbourhood = out_neigh(u);
			int l = 0;
			int r = out_degree(u) - 1;
			while(r-l > 1){
				int m = (r+l) / 2;
				NodeId w = *(neighbourhood.get(m));
				if (v < w){
					r = m;
				}
				else if (v > w){
					l = m;
				}
				else{
					return true;
				}
			}
			if(v == *(neighbourhood.get(l))){
				return true;
			}
			if(v == *(neighbourhood.get(r))){
				return true;
			}
			return false;
		}

		void prefetch_neighbourhood(NodeId v) const{
		}

		bool encoding(NodeId v) const{
				return O[v].encoding;
		}

		Kbit_Neighbourhood in_neigh(int64_t v) const {
			NodeId degree =    O[v].degree;
			int8_t bitlength = O[v].bitlength;
			int64_t ebo =      O[v].offset_or_tree.offset;
			return Kbit_Neighbourhood(degree, ebo, adjacencyArray, bitlength);
			// Bit_Tree_Neighbourhood B = Bit_Tree_Neighbourhood(degree, ebo, adjacencyArray, bitlength);
			// return * (Kbit_Neighbourhood*) &B;
			// return * (Kbit_Neighbourhood*) (new Bit_Tree_Neighbourhood(degree, ebo, adjacencyArray, bitlength));
		}

		Bit_Tree_Neighbourhood bit_tree_neigh(int64_t v) const {
		// Iterable in_neigh(int64_t v) const {
		// cointainer_type in_neigh(int64_t v) const {
			// return out_neigh(v);
			int8_t bitlength =    O[v].bitlength;
			My_Bitmap* tree  =    O[v].offset_or_tree.tree;
			return Bit_Tree_Neighbourhood(tree, bitlength);
		}


		Kbit_Neighbourhood out_neigh(int64_t v) const {
			NodeId degree =    O[v].degree;
			int8_t bitlength = O[v].bitlength;
			int64_t ebo =       O[v].offset_or_tree.offset;
			return Kbit_Neighbourhood(degree, ebo, adjacencyArray, bitlength);
		}

        /* The out-degree of a vertex v */
        NodeId out_degree(int64_t v) const {
			return O[v].degree;
		}

        /* The in-degree of a vertex v */
        NodeId in_degree(int64_t v) const {
			return out_degree(v);
		}

		Range<NodeId> vertices() const{
			return Range<NodeId>(num_nodes());
		}

		/* Get the offset of vertex v */
        int64_t getOffset(int64_t v) const {
			return offsetArray[v];
		}

		/* Creates the offset Array. This is only needed in certain algorithms
		like for example bc */
		void createOffsetArray(){
			offsetArray = (int64_t*) calloc(n+1, sizeof(int64_t));
			offsetArray[0] = 0;
			for(int v = 1; v < n; v ++){
				offsetArray[v] = offsetArray[v-1] + out_degree(v-1);
			}
		}

        int32_t* getAdjacencyArray() const {
		    return adjacencyArray;
		}

		/* Prints number of vertices, number of edges and
		 	approximate degree per vertex to the console */
		void PrintStats() const {
		    std::cout << "variable-bit per Vertex ID - graph has " << n << " nodes and "
		        << m << " ";
		    if (!isDirected){
		        std::cout << "un";
		    }
		    std::cout << "directed edges for degree: ";
		    std::cout << ((double)m/n) << std::endl;
			#if SIMPLE_GAP_ENCODING
				std::cout << "uses gap encoding" << std::endl;
			#endif
		 }

	};

#endif // Bit_Tree_Graph_H
