#ifndef Kbit_Weighted_Adjacency_Array_H
#define Kbit_Weighted_Adjacency_Array_H

#include <iostream>
#include <mmintrin.h>
#include <inttypes.h>
#include <mmintrin.h>
#include <immintrin.h>
#include <gms/third_party/gapbs/benchmark.h>
#include <map>

#include "kbit_weighted_neighbourhood.h"
#include <gms/third_party/gapbs/util.h>

#define BYTE 8

using namespace std;

class Kbit_Weighted_Adjacency_Array {
	private:
		int64_t n; // number of nodes
		int64_t m; // number of edges
        int32_t k; // number of bits to represent vertices
        int32_t w; // number of bits to represent weights
		int32_t kw; // k + w
		bool isDirected; // whether the graph is directed
        int64_t* offsetArray;
        void* adjacencyArray;
        int64_t mask;

	public:
        /* Creates an empty graph */
        Kbit_Weighted_Adjacency_Array(bool directed){
		    isDirected = directed;
		}

        /* Create a graph with n vertices and m edges. Each vertex represented with k bits */
        Kbit_Weighted_Adjacency_Array(int64_t nVertices, int32_t nBits, int32_t bitsPerWeight, int64_t nEdges,
			bool directed, int64_t* offsetArray, void* adjacencyArray){
		    n = nVertices;
		    k = nBits;
		    w = bitsPerWeight;
			m = nEdges;
			this->offsetArray = offsetArray;
			this->adjacencyArray = adjacencyArray;
		    isDirected = directed;
		    if((1 << k) < n){
		        cout << "k is too low to represent all vertex ID's" << endl;
		    }
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
			Kbit_Weighted_Neighbourhood neighbourhood = getNeighbourhood(u);
			int l = 0;
			int r = degree(u) - 1;
			while(r-l > 1){
				int m = (r+l) / 2;
				NodeId x = (*(neighbourhood.get(m))).id;
				if (v < x){
					r = m;
				}
				else if (v > x){
					l = m;
				}
				else{
					return true;
				}
			}
			if(v == (*(neighbourhood.get(l))).id){
				return true;
			}
			if(v == (*(neighbourhood.get(r))).id){
				return true;
			}
			return false;
		}

		Kbit_Weighted_Neighbourhood in_neigh(NodeId v) const {
			return Kbit_Weighted_Neighbourhood(degree(v), (k+w)*offsetArray[v], adjacencyArray, k, w);
		}

		Kbit_Weighted_Neighbourhood out_neigh(NodeId v) const {
			return Kbit_Weighted_Neighbourhood(degree(v), (k+w)*offsetArray[v], adjacencyArray, k, w);
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

		/* Returns the neighbourhood of vertex v */
		Kbit_Weighted_Neighbourhood getNeighbourhood(NodeId v) const {
			return Kbit_Weighted_Neighbourhood(degree(v), offsetArray[v], adjacencyArray, k, w);
		}

		Range<NodeId> vertices() const{
			return Range<NodeId>(num_nodes());
		}

		/* Get the offset of vertex v */
        int64_t getOffset(NodeId v) const {
			return offsetArray[v];
		}

		/* Set the degree of vertex v */
        void setOffset(NodeId v, int64_t offset){
		    offsetArray[v] = offset;
		}

        void* getAdjacencyArray() const {
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
		    std::cout << (m/n) << std::endl;
			#if SIMPLE_GAP_ENCODING
				std::cout << "uses gap encoding" << std::endl;
			#endif
		 }

};

#endif // Kbit_Weighted_Adjacency_Array_H
