#ifndef Kbit_Weighted_Adjacency_Array_Local_H
#define Kbit_Weighted_Adjacency_Array_Local_H

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

class Kbit_Weighted_Adjacency_Array_Local {
	private:
		int64_t n; // number of nodes
		int64_t m; // number of edges
		bool isDirected; // whether the graph is directed
        int64_t* O;
        int32_t* adjacencyArray;
        int64_t mask;

	public:
        /* Creates an empty graph */
        Kbit_Weighted_Adjacency_Array_Local(bool directed){
			n = 0;
			m = 0;
		    isDirected = directed;
		}

        /* Create a graph with n vertices and m edges. Each vertex represented with k bits */
        Kbit_Weighted_Adjacency_Array_Local(int64_t nVertices, int64_t nEdges,
			bool directed, int32_t* adjacencyArray, int64_t* O){
		    n = nVertices;
			m = nEdges;
			this->adjacencyArray = adjacencyArray;
			this->O = O;
		    isDirected = directed;
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
			if(out_degree(v) < out_degree(u)){
				NodeId t = u;
				u = v;
				v = t;
			}
			Kbit_Weighted_Neighbourhood neighbourhood = out_neigh(u);
			int l = 0;
			int r = out_degree(u) - 1;
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
			return out_neigh(v);
		}

		Kbit_Weighted_Neighbourhood out_neigh(int64_t v) const {
			NodeId degree =           * (int32_t*) (O + 2 * v);
			int8_t id_bitlength =     *((int8_t*)  (O + 2*v) + 4);
			int8_t weight_bitlength = *((int8_t*)  (O + 2*v) + 5);
			int64_t ebo =             * (int64_t*) (O + 2*v + 1);
			// cout << "degree : " << degree<< endl;
			// cout << "id_bitlength: " << (int)id_bitlength << endl;
			// cout << "weight_bitlength: " << (int)weight_bitlength << endl;
			// cout << "ebo: " << ebo << endl;
			return Kbit_Weighted_Neighbourhood(degree, ebo, adjacencyArray, id_bitlength, weight_bitlength);
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
        int64_t getOffset(NodeId v) const {
			return -1;
		}

        void* getAdjacencyArray() const {
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
		    std::cout << (m/n) << std::endl;
			#if SIMPLE_GAP_ENCODING
				std::cout << "uses gap encoding" << std::endl;
			#endif
		 }

};

#endif // Kbit_Weighted_Adjacency_Array_Local_H
