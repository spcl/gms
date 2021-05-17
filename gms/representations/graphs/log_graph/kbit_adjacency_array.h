#ifndef Kbit_Adjacency_Array_H
#define Kbit_Adjacency_Array_H

#include <iostream>
#include <mmintrin.h>
#include <inttypes.h>
#include <mmintrin.h>
#include <immintrin.h>

#include "kbit_neighbourhood.h"
#include <gms/third_party/gapbs/util.h>

#define BYTE 8

using namespace std;

class Kbit_Adjacency_Array {
	private:
		int64_t n; // number of nodes
		int64_t m; // number of edges
        int32_t k; // number of bits to represent vertices
		bool isDirected; // whether the graph is directed
        int64_t* offsetArray;
        int32_t* adjacencyArray;

	public:
        /* Creates an empty graph */
        Kbit_Adjacency_Array(bool directed){
		    isDirected = directed;
		}

        /* Create a graph with n vertices and m edges. Each vertex represented with k bits */
        Kbit_Adjacency_Array(int64_t nVertices, int32_t nBits, int64_t nEdges,
			bool directed, int64_t* offsetArray, int32_t* adjacencyArray){
		    n = nVertices;
		    k = nBits;
		    m = nEdges;
			this->offsetArray = offsetArray;
			this->adjacencyArray = adjacencyArray;
		    isDirected = directed;
		    if((1 << k) < n){
		        cout << "k is too low to represent all vertex ID's" << endl;
		    }
		}

		Kbit_Adjacency_Array(Kbit_Adjacency_Array &&array) :
		    n(array.n),
		    m(array.m),
		    k(array.k),
		    isDirected(array.isDirected),
		    offsetArray(array.offsetArray),
		    adjacencyArray(array.adjacencyArray)
        {
            array.offsetArray = nullptr;
            array.adjacencyArray = nullptr;
        }

		// Destructor
		~Kbit_Adjacency_Array() {
            if (offsetArray != nullptr)
			    free(offsetArray);
            if (adjacencyArray != nullptr)
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

        /* Whether the vertices u and v are connected.
			Performs binary search through all neighbours of the vertex
			with less neighbours
		*/
        bool connected(NodeId u, NodeId v) const{
			if(degree(v) < degree(u)){
				NodeId t = u;
				u = v;
				v = t;
			}
			Kbit_Neighbourhood neighbourhood = in_neigh(u);
			int l = 0;
			int r = degree(u) - 1;
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
			int64_t exactBitOffset = k * (offsetArray[v]);
			char* address = (char*)adjacencyArray + (exactBitOffset >> 3);
			// does this introduce too much overhead ?
			//int32_t cache_lines2 = ceil((degree(v)*k) / (64*8.0)) ;
			int32_t cache_lines = 1 + ((degree(v)*k) >> 9); // (64*8.0)) ;
			// cout << "degree" << degree(v)<< endl;
			// cout << "cache_lines " << cache_lines << endl;
			// cout << "cache_lines2 " << cache_lines2 << endl;
			// cache line is 64 bytes
			for(int i = 0; i < cache_lines; i++){
				_mm_prefetch(address, _MM_HINT_T0);
				address += 64;
			}
		}

		Kbit_Neighbourhood in_neigh(NodeId v) const {
			return Kbit_Neighbourhood(degree(v), k*offsetArray[v], adjacencyArray, k);
		}

		Kbit_Neighbourhood out_neigh(NodeId v) const {
			return Kbit_Neighbourhood(degree(v), k*offsetArray[v], adjacencyArray, k);
		}

        /* The degree of a vertex v */
        NodeId degree(NodeId v) const {
			return offsetArray[v+1] - offsetArray[v];
		}

        /* The out-degree of a vertex v */
        NodeId out_degree(NodeId v) const {
			return offsetArray[v+1] - offsetArray[v];
		}

        /* The in-degree of a vertex v */
        NodeId in_degree(NodeId v) const {
			return offsetArray[v+1] - offsetArray[v];
		}

		Range<NodeId> vertices() const{
			return Range<NodeId>(num_nodes());
		}

		/* Get the offset of vertex v */
        int64_t getOffset(NodeId v) const {
			return offsetArray[v];
		}

        int32_t* getAdjacencyArray() const {
		    return adjacencyArray;
		}

		int32_t bitsPerVertexID() const {
			return k;
		}

		/* Prints number of vertices, number of edges and
		 	approximate degree per vertex to the console */
		void PrintStats() const {
		    std::cout << k << "-bit per Vertex ID - graph has " << n << " nodes and "
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
