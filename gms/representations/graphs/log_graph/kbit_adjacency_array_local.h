#ifndef Kbit_Adjacency_Array_Local_H
#define Kbit_Adjacency_Array_Local_H

#include <iostream>
#include <mmintrin.h>
#include <inttypes.h>
#include <mmintrin.h>
#include <immintrin.h>

#include "kbit_neighbourhood.h"
#include <gms/third_party/gapbs/util.h>

#define BYTE 8

using namespace std;

/* The is the implementation of Log(Graph) where every neighbourhood
 * can be encoded with different bitlength.
 * Besides the edge that that is stored in the 'adjacencyArray'
 * we also have to store the bit offset for each neighbourhood,
 *  - the bit offset in the adjacency array where the neighbourhood is encoded
 *  - the edge index, which is needed to calculate the degree of vertices
 *  - the bitlength with which the neighbourhood is encoded.
 * Design Decisions:
 * The edge index and the bit offset are encoded in the same array:
 * at the locations '2*v' and '2*v+1' respectively for a vertex 'v'
 * The bitlength is encoded in the last 5 bits of the bit offset.
*/

class Kbit_Adjacency_Array_Local {
	private:
		int64_t n; // number of nodes
		int64_t m; // number of edges
		bool isDirected; // whether the graph is directed
        int64_t* O; // array that contains degree, bit offsets and bitlength
        int32_t* adjacencyArray;
		int64_t* offsetArray; // contains edge offsets (only needed for bc)
		const int64_t mask_5 = ~((int64_t) -1 << 5); // selects first 5 bits
		const int64_t mask_59 = ~((int64_t) -1 << 59); // selectes first 59 bits

	public:
        /* Creates an empty graph */
        Kbit_Adjacency_Array_Local(bool directed){
		    isDirected = directed;
		}

        /* Create a graph with n vertices and m edges. Each vertex represented with k bits */
        Kbit_Adjacency_Array_Local(int64_t nVertices, int64_t nEdges,
			bool directed, int32_t* adjacencyArray, int64_t* O){
		    n = nVertices;
		    m = nEdges;
			this->O = O;
			this->adjacencyArray = adjacencyArray;
		    isDirected = directed;
		}

		Kbit_Adjacency_Array_Local(Kbit_Adjacency_Array_Local &&array) :
		    n(array.n),
		    m(array.m),
		    isDirected(array.isDirected),
		    O(array.O),
		    adjacencyArray(array.adjacencyArray),
		    offsetArray(array.offsetArray)
		{
            array.O = nullptr;
            array.adjacencyArray = nullptr;
        }

		// Destructor
		~Kbit_Adjacency_Array_Local() {
            if (O != nullptr) free(O);
            if (adjacencyArray != nullptr) free(adjacencyArray);
            // TODO delete offset array too?
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
			Kbit_Neighbourhood neighbourhood = in_neigh(u);
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

		Kbit_Neighbourhood in_neigh(int64_t v) const {
			NodeId degree =    * (int32_t*) (O + 2 * v);
			int8_t bitlength = *((int32_t*) (O + 2*v) + 1);
			int64_t ebo =       * (int64_t*) (O + 2*v +1);
			return Kbit_Neighbourhood(degree, ebo, adjacencyArray, bitlength);
		}

		Kbit_Neighbourhood out_neigh(int64_t v) const {
			NodeId degree =    * (int32_t*) (O + 2 * v);
			int8_t bitlength = *((int32_t*) (O + 2*v) + 1);
			int64_t ebo =       * (int64_t*) (O + 2*v +1);
			return Kbit_Neighbourhood(degree, ebo, adjacencyArray, bitlength);
		}

        /* The out-degree of a vertex v */
        NodeId out_degree(int64_t v) const {
			return *(int32_t*) (O + 2*v);
		}

        /* The in-degree of a vertex v */
        NodeId in_degree(int64_t v) const {
			return *(int32_t*) (O + 2*v);
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

#endif // Kbit_Adjacency_Array_H
