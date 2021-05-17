#ifndef COLORING_DENSE_SPARSE_H_
#define COLORING_DENSE_SPARSE_H_
#include <omp.h>
#include <cassert>
#include <random>

#include "gms/third_party/gapbs/benchmark.h"
#include "gms/third_party/gapbs/builder.h"
#include "gms/third_party/gapbs/command_line.h"
#include "gms/third_party/gapbs/graph.h"

#include "coloring_common.h"
#include "random_select.h"

#include "coloring_barenboim.h"

namespace GMS::Coloring {

typedef NodeId ComponentID;

template <class CGraph>
class Coloring_Dense_Sparse {
private:
    // in - out:
    CGraph &g;
    std::vector<ColorID> &coloring;
    // const AlgoParameters& algoParams;

    // constants from paper
    const double K = 100; // sufficiently large (hope is big enough !?)
    const double C = 1.0/K/6.0; // small >0, 6C at most 1/K
    double epsilon;
    double alpha = 0.01; // initial coloring probability

    // our own constants
    double friendBeta = 0.01; // prob: if check for friendEdge
    double friendBetaThreashold = 0.5; // for density

    // local states:
    DetailTimer detailTimer;
    int64_t n;

    int64_t delta;
    int64_t deltaM;
    int64_t min1Delta;
    double friendNumberPaper;
    double friendNumber;

    // basically copy of g, but only has friend edges
    std::vector<uint64_t> friendGraphOffset;
    std::vector<NodeId> friendGraph;
    std::vector<NodeId> friendEdgeCounter;

    // denseGraph
    const NodeId invalidNode = -1;
    NodeId nDense;
    std::vector<NodeId> denseToGraph; // v -> vDense || invalidNode
    std::vector<NodeId> graphToDense; // vDense -> v

    std::vector<NodeId> denseGraph; // Adjacency list format
    // uses same vector as friendGraph (swapped at some point)
    std::vector<NodeId> denseGraphEdgeIndex; // offset for vDense in denseGraph
    std::vector<NodeId> denseGraphEdgeCount; // offset for vDense in denseGraph

    // dense Components
    std::vector<ComponentID> denseComponent; // vDense -> componentID
    ComponentID nComponent;

    std::vector<NodeId> componentMembers;
    std::vector<NodeId> componentCount; // componentID -> count, decreases over time as get colored
    std::vector<NodeId> componentMembersOffset; // componentID -> where list starts
public:
    Coloring_Dense_Sparse(CGraph& _g, std::vector<ColorID> &_coloring)
    : g(_g), coloring(_coloring)
    {
	    n = g.num_nodes();


        // need sorted neighborhoods for friend-edge finding
        sort_graph_neighborhoods(g);
        assert(is_sorted(g));

        int64_t delta_ = 0;
        int64_t deltaM_ = n;

        #pragma omp parallel for reduction(max:delta_) reduction(min:deltaM_)
        for(NodeId v=0;v<n;v++) {
            assert(g.out_degree(v) == g.in_degree(v));
            delta_ = std::max(delta_,g.out_degree(v));
            deltaM_ = std::min(deltaM_,g.out_degree(v));
        }
        delta = delta_;
        deltaM = deltaM_;

        // std::cout << "-- Constants and Graph properties:" << std::endl;
        // std::cout << "degree: max " << delta << ", min " << deltaM << std::endl;

        // constants from paper
        epsilon = C * std::pow(100.0,-std::sqrt(std::log((double)delta)));

        // std::cout << "C: " << C << std::endl;
        // std::cout << "K: " << K << std::endl;
        // std::cout << "epsilon from paper: " << epsilon << std::endl;

        // reflection about this condition below:
        const double conditionFraction = std::pow(epsilon,4)*delta / (K*std::log(n));
        if( conditionFraction < 1.0 ) {
            // std::cout << "K: condition not satisfied, just run [9] Barenboim, according to paper." << std::endl;
            // std::cout << "K: condition off by factor " << conditionFraction << std::endl;
        } else {
            // std::cout << "K: assumption on K is satisfied, algo makes sense here." << std::endl;
        }

        min1Delta = std::max(1l,delta);
        friendNumberPaper = (1.0 - epsilon) * delta;
        // std::cout << "friendNumber from paper: " << friendNumberPaper << std::endl;

        // std::cout << "-- Parameters (set with -p 'n1=v1,n2=v2'):" << std::endl;

        double epsilonApplied = epsilon;//algoParams.getDouble("epsilon",epsilon);
        // std::cout << "epsilon applied: " << epsilonApplied << std::endl;

        // if(epsilonApplied > 0.2) {std::cout << "  WARNING: epsilon should not be higher than 0.2, according to paper!" << std::endl;}
            assert(epsilonApplied > 0);
        friendNumber = std::ceil((1.0 - epsilonApplied)*min1Delta); // epsilon <= 1/5 for small dense component diameters
            // std::cout << "friendNumber applied: " << friendNumber << " (derived)" << std::endl;

        alpha = 0.01;//algoParams.getDouble("alpha",0.01);
            // std::cout << "alpha: " << alpha << " (initial coloring prob)" << std::endl;

        friendBeta = 1.0;//algoParams.getDouble("beta",1.0);
        friendBetaThreashold = 1.0;//algoParams.getDouble("betaT",1.0);
        //     std::cout << "beta: " << friendBeta << " (prob to check edge if friend-edge)"<< std::endl;
        // std::cout << "betaT: " << friendBetaThreashold << " (beta threashold)" << std::endl;

        // std::cout << "--" << std::endl;

        // detailTimer.endPhase("init");
    }

    void decomposition() {
        decomposition_friend_edges();
        if(nDense>0) {
            decomposition_dense_graph();
            decomposition_components();
        }
    }

    void decomposition_friend_edges() {
	    friendGraphOffset.resize(n+1,0);

        #pragma omp parallel for
        for(uint64_t i=0; i<n; i++) {
            friendGraphOffset[i] = g.out_neigh(i).begin() - g.out_neigh(0).begin();
        }
	    friendGraphOffset[n] = g.num_edges()*2;

        friendGraph.resize(friendGraphOffset[n]);
        friendEdgeCounter.resize(n,0);

        // Idea: could do graphToDense via settin 1, prefixsum
        // remove fetch_and_add, maybe faster?
        graphToDense.resize(n,invalidNode);
        nDense = 0;

            // Idea: dynamic, so that better balance?
        // was true, especially for degree_ordered graphs
        // seems not to have had performance impact elsewhere

        // other Idea:
        // half time by only considering edges from low->high.
        // Then find other edges for high ID later
        //  - or: just do atomically!
        //  - or: write 0/1 into friendEdge, write 1 if want edge.
        //       - must write into others friendEdge, so some cache issues, but could work out.

        // other idea:
        // combine component finding and friend edges?
        // somehow collect almost cliques?
        //
        // - when getting friend edge: continue around it (already know shared neighbors)
        //     - hmm not sure can save
        // - since need many friendEdges to be dense, overlap large
        // - assume you are leader of comp. find what is yours (diam 2 neighbors).
        //     - take all in 2 neighborhood (max delta squared)
        //     - perform friend edge tests there
        //     - if find smaller vertex in 2 neighborhood:
        //        - must see if in same component?
        //        - check if I am even dense? - relies on lots of friend edges...
        // - check number in 2-neighborhood vs sum over number of neighbors of neighbors?
        //     - could this give me if I am dense? if small N, but large S, know strongly interconnected
        //     - how find N efficiently?
        // - vote for edges somehow? is friend edge if enough votes?
        //     - go over triangles? - probably same time issues?

            // new Idea: sub-sample: toss coin if even consider edge
        // when consider dense?
        // if has friendBeta*friendNumber friendEdges?
        //    - maybe slightly less bc variance.

        #pragma omp parallel
	    {
            std::mt19937 rng(omp_get_thread_num());
            std::bernoulli_distribution coin(friendBeta);

            #pragma omp for schedule(dynamic,8)
            for (NodeId v = 0; v < n; v++) {
    	        NodeId lastU = 0;

                if(g.out_degree(v)>=friendNumber) {
                    auto N = g.out_neigh(v);
                    for(NodeId* it = N.begin(); it<N.end(); it++) {
                        const NodeId u = *it;
                        if(v<u && g.out_degree(u)>=friendNumber && coin(rng)) {
                            assert(lastU<=u); // guard against multiple edges
                            lastU = u+1;

                            NodeId* uNit = g.out_neigh(u).begin();
                            NodeId* vNit = g.out_neigh(v).begin();
                            NodeId* uNbeg = uNit;
                            const NodeId* uNend = g.out_neigh(u).end();
                            const NodeId* vNend = g.out_neigh(v).end();

                            size_t sharedNeighbors = 0;
                            // Idea: abort as soon as cannot have enough shared.
                            while(uNit < uNend && vNit < vNend) {
                                if(*uNit == *vNit) {
                                    // match
                                    sharedNeighbors++;
                                    uNit++;vNit++;
                                } else if (*uNit < *vNit) {
                                    uNit++;
                                } else {
                                    vNit++;
                                }
                            }
                            if(sharedNeighbors >= friendNumber) {
                                // mark edge as to keep:
                                const uint64_t uIndex = it - N.begin();// where is u in Nv
                                NodeId* uIt = uNbeg;
                                while(*uIt != v) {uIt++;}
                                const uint64_t vIndex = uIt - uNbeg;// where is v in Nu
                                friendGraph[friendGraphOffset[u] + vIndex] = 1;
                                friendGraph[friendGraphOffset[v] + uIndex] = 1;
                            }
                        }
                    }
    	        }
            }// for
	    }// omp parallel

	// now scan and transform, cound friend edges:
	#pragma omp parallel for schedule(dynamic,8)
    for (NodeId v = 0; v < n; v++) {
        if(g.out_degree(v)>=friendNumber) {
            auto N = g.out_neigh(v);
            NodeId* feBegin = &friendGraph[friendGraphOffset[v]];
            NodeId* feIt = feBegin;
            NodeId* feWrite = feBegin;
            for(NodeId* it = N.begin(); it<N.end(); it++) {
                const NodeId u = *it;
                    if(*feIt) {
                        *(feWrite++) = *it; // write u into friendGraph at next pos
                }
                feIt++;
            }
            NodeId feCount = feWrite - feBegin; // how many were written
            if(feCount>=friendNumber*friendBeta*friendBetaThreashold){
                friendEdgeCounter[v] = feCount;
                const NodeId vDense = fetch_and_add(nDense,1);//atomic
                graphToDense[v] = vDense;
                // Idea: if fetch_and_add too expensive:
                // could do prefix sum over graphToDense
            }
        }
    }
	// std::cout << "nDense: " << nDense << std::endl;
    //     detailTimer.endPhase("decomp_friend_edge");
    }

    void decomposition_dense_graph() {
        denseToGraph.resize(nDense);

        // take friendGraph, condense to denseGraph
	// keep only dense edges, write vDense instead of v.

        #pragma omp parallel for
	    for (NodeId v = 0; v < n; v++) {
            NodeId vDense = graphToDense[v];
            if(vDense!=invalidNode) {
                denseToGraph[vDense] = v;

                //std::cout << vDense << " " << v << std::endl;

                // only keep friend edges to dense vertices, write as such:
                const int64_t vOffsetBegin = friendGraphOffset[v];
                const int64_t vOffsetEnd = friendGraphOffset[v]+friendEdgeCounter[v];
                std::sort(friendGraph.begin()+vOffsetBegin, friendGraph.begin()+vOffsetEnd); // sort adjacencies
                NodeId nWrite = 0;
                for(int64_t o = vOffsetBegin; o<vOffsetEnd; o++) {
                    NodeId u = friendGraph[o];
                    NodeId uDense = graphToDense[u];
                    if(uDense!=invalidNode) {
                        friendGraph[vOffsetBegin+nWrite++] = uDense;
                        //std::cout << "  " << u << " " << uDense << std::endl;
                    }
                }
                friendEdgeCounter[v] = nWrite;
            }
        }


        denseGraphEdgeIndex.resize(nDense);
        denseGraphEdgeCount.resize(nDense);

        #pragma omp parallel for
	    for(NodeId vDense = 0; vDense < nDense; vDense++){
            NodeId v = denseToGraph[vDense];
            denseGraphEdgeIndex[vDense] = friendGraphOffset[v];
            denseGraphEdgeCount[vDense] = friendEdgeCounter[v];
        }
        std::swap(denseGraph,friendGraph); // now transformation has happened

        // Idea: could free up mem for:
	// friendGraphOffset
	// friendEdgeCounter

	// debug print
	//for(NodeId vDense = 0; vDense < nDense; vDense++){
	//    NodeId v = denseToGraph[vDense];
	//    const int64_t vOffsetBegin = denseGraphEdgeIndex[vDense];
	//    const int64_t vOffsetEnd = vOffsetBegin + denseGraphEdgeCount[vDense];
	//    std::cout << vDense << " " << v << std::endl;
	//    for(int64_t o = vOffsetBegin; o<vOffsetEnd; o++) {
	//        NodeId uDense = denseGraph[o];
	//	NodeId u = denseToGraph[uDense];
	//	std::cout << "   " << uDense << " " << u << std::endl;
	//    }
	//}

        // detailTimer.endPhase("decomp_dense_graph");
    }

    void decomposition_components() {
        assert(nDense > 0);

        denseComponent.resize(nDense);
        NodeId iComponent = 0; // acquire new id here

        // just make big, later adjust
	    componentCount.resize(nDense);

        #pragma omp parallel // num_threads(1)
	    {
            std::vector<NodeId> bfsQueue(nDense,invalidNode); // local
            std::vector<NodeId> bfsVisited(nDense,invalidNode); // local

	    // Idea: make dynamic, small block size because all prob small ?
            #pragma omp for
	        for(NodeId vDense = 0; vDense < nDense; vDense++){
                NodeId bfsInsert = 0;
                NodeId bfsProcess = 0;
                bfsQueue[bfsInsert++] = vDense; // push first
                bfsVisited[vDense] = vDense;
                bool doAbort = false;
		        int64_t nEdges = 0;
                while((! doAbort) && bfsProcess < bfsInsert) {
                    const NodeId wDense = bfsQueue[bfsProcess++];//take next

                    // visit it's neighbors
                    const int64_t uOffsetBegin = denseGraphEdgeIndex[wDense];
                    const int64_t uOffsetEnd = uOffsetBegin + denseGraphEdgeCount[wDense];
                    nEdges += denseGraphEdgeCount[wDense];
                    for(uint64_t o = uOffsetBegin; o < uOffsetEnd && (!doAbort); o++) {
                        const NodeId uDense = denseGraph[o];
                        if(uDense < vDense) {
                            // abort, found a smaller index
                            doAbort = true;
                        }
                        if(uDense > vDense && bfsVisited[uDense]!=vDense) { // not yet visited by vDense
                            bfsQueue[bfsInsert++] = uDense; // push new one
                            bfsVisited[uDense] = vDense; // mark as visited by vDense
                        }
                    }
                }
                if(!doAbort) {
                    ComponentID componentID = fetch_and_add(iComponent,1);
                    componentCount[componentID] = bfsInsert;
                    // std::cout << componentID<< " I am leader: " << vDense << " " << denseToGraph[vDense]
			    //   << " size: " << bfsInsert << " edges: " << nEdges << std::endl;
                    for(uint64_t i = 0; i<bfsInsert; i++) {
                        denseComponent[bfsQueue[i]] = componentID;
                        //std::cout << "member: " << bfsQueue[i] << std::endl;
                    }
                }
            }
	    }

        nComponent = iComponent;
        // std::cout << "nComponents: " << nComponent << std::endl;

        componentMembersOffset.resize(nComponent);
        // Idea: prefix sum - also for inside comp
        componentMembersOffset[0] = 0;
        for(ComponentID comp = 1; comp < nComponent; comp++){
            componentMembersOffset[comp] = componentMembersOffset[comp-1] + componentCount[comp-1];
        }

        // memory preservation trick below:
        componentMembers.resize(nComponent);
        std::swap(componentMembers,componentCount);
            #pragma omp parallel for
        for(ComponentID comp = 0; comp < nComponent; comp++){
            componentCount[comp] = componentMembers[comp];
        }

        // just for debugging:
            #pragma omp parallel for
        for(int64_t i = 0; i<nDense; i++) {
            componentMembers[i] = -1;
        }

	// Idea: prefix sum - together with comp-offset
        #pragma omp parallel for
	    for(ComponentID comp = 0; comp < nComponent; comp++){
            uint64_t next = componentMembersOffset[comp];
                for(NodeId vDense = 0; vDense < nDense; vDense++){
                    if(denseComponent[vDense] == comp) {
                        componentMembers[next++] = vDense;
                    }
                }
            assert(next <= componentMembersOffset[comp] + componentCount[comp]);
            componentCount[comp] = next - componentMembersOffset[comp];
        }

	// validate result:
	    assert(true);
        #pragma omp parallel for
        for(NodeId comp = 0; comp < nComponent; comp++) {
            const NodeId compSize = componentCount[comp];
            assert(compSize>0);

            const uint64_t compOffsetBegin = componentMembersOffset[comp];
            const uint64_t compOffsetEnd = compOffsetBegin + compSize;
            for(uint64_t o = compOffsetBegin; o<compOffsetEnd; o++) {
                const NodeId vD = componentMembers[o];
                // if(denseComponent[vD] != comp) {
                //     std::cout << "validate1 comp: " << comp << " vD: " << vD << " realComp: " << denseComponent[vD] << std::endl;
                //     std::cout << "b " << compOffsetBegin << " o " << o << " e " << compOffsetEnd << std::endl;
                // }
                // assert(denseComponent[vD] == comp);
            }
	    }

        // reinsert friendEdges between v of same comp
	//   - dropped bc friendBeta sampling

        #pragma omp parallel for
	    for(NodeId vDense = 0; vDense < nDense; vDense++){
            NodeId v = denseToGraph[vDense];
            ComponentID comp = denseComponent[vDense];
            NodeId* beginIt = &denseGraph[denseGraphEdgeIndex[vDense]];
            NodeId* writeIt = beginIt;

            for(auto u : g.out_neigh(v)) {
                NodeId uDense = graphToDense[u];
                if(uDense!=invalidNode) {
                    ComponentID uComp = denseComponent[uDense];
                    if(comp==uComp) {
                        *(writeIt++) = uDense;
                    //std::cout << "edge recovered: " << vDense << " " << uDense << std::endl;
                    }
                }
            }
            //std::cout << "from " << denseGraphEdgeCount[vDense] << " to "
                //    << (writeIt - beginIt) << std::endl;
            denseGraphEdgeCount[vDense] = writeIt - beginIt;

        }
	//// debug print
	//std::cout << "Debug Print componentns:" << std::endl;
	//for(ComponentID comp = 0; comp < nComponent; comp++){
	//    int64_t begin = componentMembersOffset[comp];
        //    for(int64_t o = begin; o < begin + componentCount[comp]; o++){
        //        NodeId vDense = componentMembers[o];
	//	std::cout << comp << " " << vDense << std::endl;
	//    }
	//}

        // detailTimer.endPhase("decomp_components");
    }

    void initial_coloring() {
        // Assume that palette at beginning is [Delta+1] for all vertices
        // with probability alpha = 0.01, pick a random color. Else keep color 0
        //  - conflict resolution: keep color if no other wants the same. Else set back to 0
        if(nDense == 0){return;}

        std::vector<ColorID> tmpColoring(n,0);

        uint64_t successes = 0;
        // Idea: dynamic for better balance?
        #pragma omp parallel reduction(+:successes)
        {
            std::mt19937 rng(omp_get_thread_num());
            std::uniform_int_distribution<NodeId> udist(1, delta+1);
            std::bernoulli_distribution shouldColor(alpha);

            // tentative coloring round
            #pragma omp for
            for (NodeId v = 0; v < n; v++) {
                if(shouldColor(rng)) {
                    tmpColoring[v] = udist(rng);
                }
            }

            // conflict resolution round
            #pragma omp for
            for (NodeId v = 0; v < n; v++) {
                ColorID c = tmpColoring[v];
                if(c>0) {
            	    bool keepColor = true;
                    for (NodeId u : g.out_neigh(v)) {
                        if(coloring[u] == c) {
                            keepColor = false;
                            break;
                        }
                    }
            	    if(keepColor) {
                        coloring[v] = c;
                        successes++;
                        //std::cout << "init col: " << v << " " << graphToDense[v] << std::endl;
                    } // commit if no conflict
                }
            }
        }
        // std::cout << "init col successes: " << successes << std::endl;

        // removal of colored component members:
        #pragma omp parallel for
        for(NodeId comp = 0; comp < nComponent; comp++) {
            const NodeId compSize = componentCount[comp];
            assert(compSize>0);

            const NodeId compOffsetBegin = componentMembersOffset[comp];
            const NodeId compOffsetEnd = compOffsetBegin + compSize;
            NodeId writeIndex = 0;
            for(NodeId o = compOffsetBegin; o<compOffsetEnd; o++) {
                const NodeId vD = componentMembers[o];
            // if(denseComponent[vD] != comp) {
            //     std::cout << "comp: " << comp << " vD: " << vD << " realComp: " << denseComponent[vD] << std::endl;
            // }
                assert(denseComponent[vD] == comp);
                const NodeId v = denseToGraph[vD];
                const ColorID vCol = coloring[v];
                if(vCol == 0) { // keep (else overwrite)
                    componentMembers[compOffsetBegin + writeIndex++] = vD;
                }
            }
            componentCount[comp] = writeIndex; // number of kept members
        }

        // detailTimer.endPhase("init_coloring");
    }

    void coloring_steps() {
        if(nDense == 0){return;}
	//  - need external degree (degree of edges to outside component) - dBar(v)i - at most: epsilon delta
        //  - need anti-degree (non-neighbors of v in same component) a(v)i - at most: 3 epsilon delta
        //     - Di >= than both
        //  - Zi at most palette size of dense vertices
	    const uint64_t nStepsDenseColoring = std::ceil(std::log((double) delta));
        // std::cout << "nStepsDenseColoring: " << nStepsDenseColoring << std::endl;

        // must use color palettes:
        // need easy random select, but also easy removal.
        // could just delete (mark deleted, decr if needed)
        // cleanup on next select, keep boundary up to which clean?
        // or cheaper to just move all? - for now just do this.
        // but find is not cheap (well log, so ok)

        std::vector<std::vector<ColorID>> palettes(nDense);
        std::vector<NodeId> externalDegree(nDense,0);
        std::vector<NodeId> internalDegree(nDense,0);
        // anti-degree: component_size - internalDegree
        #pragma omp parallel for
	    for(NodeId vD = 0; vD < nDense; vD++){
            palettes[vD].resize(delta+1);
            std::generate(palettes[vD].begin(), palettes[vD].end(), [c = 1]() mutable { return c++; });

            const NodeId v = denseToGraph[vD];
            const NodeId vComp = denseComponent[vD];
            for (NodeId u : g.out_neigh(v)) {
                const ColorID uColor = coloring[u];
                if(uColor == 0) {// note still uncolored
                    // count as neighbor
                    const NodeId uD = graphToDense[u];
                    if(uD == invalidNode) {
                        // u is sparse node
                        externalDegree[vD]++;
                        //std::cout << "init eD++ " << vD << " " << uD << std::endl;
                    } else {
                        // u is dense node
                        const NodeId uComp = denseComponent[uD];
                        if(vComp == uComp) {
                            internalDegree[vD]++;
                            //std::cout << "init iD++ " << vD << " " << uD << std::endl;
                        } else {
                        externalDegree[vD]++;
                            //std::cout << "init eD++ " << vD << " " << uD << std::endl;
                        }
                    }
                } else {
                    // update color palette
            	    auto palette_v_end = std::remove(palettes[vD].begin(), palettes[vD].end(), uColor);
                    palettes[vD].resize(std::distance(palettes[vD].begin(), palette_v_end));
                }
            }
            //std::cout << "prep dense: " << vD << " eD: " << externalDegree[vD] << " iD: " << internalDegree[vD] << " pal: " << palettes[vD].size() << std::endl;
        }


        // detailTimer.endPhase("coloring_steps_prep");


        for(int i = 0; i< nStepsDenseColoring; i++) {
            // std::cout << "Starting next dense coloring round: " << i << std::endl;

            // for each component
            //  - pick L vertices at random
            //  - color with random color from palette
            //  - if conflict with color from other component, keep only color of smaller componentID
            std::vector<ColorID> tmpColoring(nDense,0);
            // what to do about random accesses? use same order as in componentMembers - could change...but so what, only relevant for this round!

            // set after conflict resolution and before commit
            std::vector<ColorID> commitColoring(nDense,0);

            #pragma omp parallel
	        {
                int stride = std::max(omp_get_num_threads(), (int)nStepsDenseColoring);
                std::mt19937 rng(omp_get_thread_num()*stride + i);

                // IDEA: make dynamic!
                #pragma omp for schedule(dynamic,8)
		        for(NodeId comp = 0; comp < nComponent; comp++) {
                    // find Di and Zi
                    NodeId Di = 1;
                    NodeId Zi = delta;
                    const NodeId compSize = componentCount[comp];

                    if(compSize == 0) {continue;}
                    assert(compSize>0);

                    const int64_t compOffsetBegin = componentMembersOffset[comp];
                    const int64_t compOffsetEnd = compOffsetBegin + compSize;
                    for(int64_t o = compOffsetBegin; o<compOffsetEnd; o++) {
                        const NodeId vD = componentMembers[o];
                        Di = std::max(Di, std::max(externalDegree[vD] , componentCount[comp]-internalDegree[vD]));
                        const NodeId Qiv = palettes[vD].size();
                        Zi = std::min(Zi, Qiv);

                //     if(denseComponent[vD] != comp) {
                //     std::cout << "comp: " << comp << " vD: " << vD << " realComp: " << denseComponent[vD] << std::endl;
                // }
                        assert(denseComponent[vD] == comp);
                    }

                    const double DbyZ = (double)Di / (double)Zi;
                    const double ZbyD = (double)Zi / (double)Di;
                    const NodeId L = std::ceil((double)componentCount[comp] * (1.0-2.0*DbyZ*std::log(ZbyD)));
                    //std::cout << "comp: " << comp << " Size: " << componentCount[comp] << " Di: " << Di << " Zi: " << Zi << " L: " << L << std::endl;

                    //std::cout << "CompInit: " << comp << " size: " << compSize << " Di: " << Di
                    //	      << " Zi: " << Zi << " L: " << L << std::endl;
                    if(Di == 0 || Zi == 0 || L == 0 || compSize == 0) {
                        for(int64_t o = compOffsetBegin; o<compOffsetEnd; o++) {
                            const NodeId vD = componentMembers[o];
                            // std::cout << "vD: " << vD << " eD: " << externalDegree[vD]
                            //   << " iD: " << internalDegree[vD] << std::endl;
                        }
                    }

                    assert(Di > 0);
                    assert(Zi > 0);
                    assert(L > 0 && L <= compSize);

                    // select L from component
                    std::vector<NodeId> permutation(componentCount[comp]);
                    std::generate(permutation.begin(), permutation.end(), [c = 0]() mutable { return c++; });
                    std::shuffle(permutation.begin(),permutation.end(),rng);
                    for(NodeId i = 0; i<L; i++) {
                            NodeId pi = permutation[i];
                        const NodeId o = componentMembersOffset[comp] + pi;
                            const NodeId vD = componentMembers[o];

                        // pick random color from palette, but cannot be one of neighbor just chosen.
                        bool retry = true;
                        while(retry) {
                                retry = false;
                            const NodeId palIndex = std::uniform_int_distribution<NodeId>{0, palettes[vD].size()-1}(rng);
                            const ColorID cCandidate = palettes[vD][palIndex];

                            // for all neighbors in comp:
                            const uint64_t index_begin = denseGraphEdgeIndex[vD];
                            const uint64_t index_end = index_begin + denseGraphEdgeCount[vD];
                            for(uint64_t uOffset = index_begin; uOffset < index_end && (! retry); uOffset++) {
                                const NodeId uD = denseGraph[uOffset];
                                    //const NodeId u = denseToGraph[uD];
                                assert(denseComponent[uD]==denseComponent[vD]);

                                //const uCol = coloring[u];
                                const ColorID uColTmp = tmpColoring[vD];
                                if(cCandidate==uColTmp) {
                                    // reject cCandidate
                                    retry = true;
                                }
                            }
                            if(!retry) {tmpColoring[vD] = cCandidate;}
                        }
                    }
                }

                // validate tmp colors, only take if no other vertex has same (only possible if from other component)
                // would it make sense to store these edges in dedicated structure?
                #pragma omp for schedule(dynamic,8)
		        for(NodeId vD = 0; vD < nDense; vD++) {
                    const NodeId v = denseToGraph[vD];
                    const ColorID cTmp = tmpColoring[vD];
                    if(cTmp!=0) { // for all that have just been tmp colored
                        const NodeId v = denseToGraph[vD];
                        bool hasConflict = false;
                        for (NodeId u : g.out_neigh(v)) {
                            const NodeId uD = graphToDense[u];
                            if(uD != invalidNode) {// u is dense
                                if(tmpColoring[uD] == cTmp && v > u) {hasConflict = true;}
                            }
                        }
                        if(hasConflict) {
                            // reject color
                            //std::cout << vD << " cTmp:" << cTmp << " - rejected"<< std::endl;
                        } else {
                            // commit color, update things
                            //std::cout << vD << " cTmp:" << cTmp << " - commit"<< std::endl;
                            commitColoring[vD] = cTmp;
                        }
                    }
                }

                // commit color and update internalDegree and externalDegree and palletes
                #pragma omp for schedule(dynamic,8)
		        for(NodeId vD = 0; vD < nDense; vD++) {
                    const NodeId v = denseToGraph[vD];
                    const ColorID col = commitColoring[vD];
                    const NodeId vComp = denseComponent[vD];
                    if(col!=0) { // if color commited
                        coloring[v] = col;
                    } else {
                        // update things:
                        for (NodeId u : g.out_neigh(v)) {
                            const NodeId uD = graphToDense[u];
                            if(uD != invalidNode) {// u is dense
                                const ColorID uColCommit = commitColoring[uD];
                                if(uColCommit != 0) {
                                    const NodeId uComp = denseComponent[uD];
                                    if(vComp == uComp) {
                                        internalDegree[vD]--;
                                            //std::cout << "iD-- " << vD << " " << uD << std::endl;
                                    } else {
                                        externalDegree[vD]--;
                                            //std::cout << "eD-- " << vD << " " << uD << std::endl;
                                    }
                                    auto palette_v_end = std::remove(palettes[vD].begin(), palettes[vD].end(), uColCommit);
                                    palettes[vD].resize(std::distance(palettes[vD].begin(), palette_v_end));
                                }
                            }
                        }
                    }
                }

		        // removal of component members:
                #pragma omp for schedule(dynamic,8)
		        for(NodeId comp = 0; comp < nComponent; comp++) {
                    const NodeId compSize = componentCount[comp];
                    if(compSize == 0) {continue;}
		            assert(compSize>0);

                    const NodeId compOffsetBegin = componentMembersOffset[comp];
                    const NodeId compOffsetEnd = compOffsetBegin + compSize;
                    NodeId writeIndex = 0;
                    for(NodeId o = compOffsetBegin; o<compOffsetEnd; o++) {
                        const NodeId vD = componentMembers[o];
                        const ColorID vCol = commitColoring[vD];
                        if(vCol == 0) { // keep (else overwrite)
                            componentMembers[compOffsetBegin + writeIndex++] = vD;
                        }
                    }
                    componentCount[comp] = writeIndex; // number of kept members
                }


                if(false){
		            // validate updates, can remove later
                    #pragma omp for
		            for(NodeId comp = 0; comp < nComponent; comp++) {
                        const NodeId compSize = componentCount[comp];

                        if(compSize == 0) {continue;}
                        assert(compSize>0);

                        const NodeId compOffsetBegin = componentMembersOffset[comp];
                        const NodeId compOffsetEnd = compOffsetBegin + compSize;
                        for(NodeId o = compOffsetBegin; o<compOffsetEnd; o++) {
                            const NodeId vD = componentMembers[o];
                            NodeId intDeg = 0;
                            NodeId extDeg = 0;
                            const NodeId v = denseToGraph[vD];

                            for (NodeId u : g.out_neigh(v)) {
                                if(coloring[u] == 0) {
                        	        const NodeId uD = graphToDense[u];
                        	        if(uD != invalidNode) {
                        	            const NodeId uComp = denseComponent[uD];
                                        if(uComp == comp) {
                        	    	        intDeg++; // dense of same comp
                    	                //std::cout << "valid iD " << vD << " " << uD << std::endl;
                        	    	    } else {
                        	    	        extDeg++; // dense of other comp
                    	                    //std::cout << "valid eD " << vD << " " << uD << std::endl;
                        	    	    }
                        	        } else {
                        	    	    extDeg++; // sparse
                    	            //std::cout << "valid eD " << vD << " " << uD << std::endl;
                        	        }
                        	    }
                            }
                            //std::cout << "validate: " << vD << " " << intDeg << " " << internalDegree[vD]
                        	//      << " " << extDeg << " " << externalDegree[vD] << std::endl;
                            // if(intDeg != internalDegree[vD] || extDeg != externalDegree[vD]) {
                            //         std::cout << "validate: " << vD << " " << v
                            // << " " << intDeg << " " << internalDegree[vD]
                            // 	    << " " << extDeg << " " << externalDegree[vD]
                            // 	    << " comp " << comp << std::endl;
                            // }
                            assert(intDeg == internalDegree[vD]);
                            assert(extDeg == externalDegree[vD]);
                        }
                    }
		        }
	        }// pragma omp parallel


            std::string phaseName("coloring_step_");
            phaseName += std::to_string(i);
            detailTimer.endPhase(phaseName.c_str());
        }
    }

    // ~Coloring_Dense_Sparse() {
    //     // detailTimer.endPhase("rest");
    //     detailTimer.print();
    // }

    int n_colors() {return n;}

    // void print_nColored() {
    // 	// debug counting below:
	//     uint64_t nColored = 0;
    //     #pragma omp parallel for reduction(+:nColored)
    //     for(uint64_t i =0; i<n; i++) {
    //         if(coloring[i]>0){nColored++;}
    //         assert(coloring[i] <= delta+1);
    //     }
	// // std::cout << "nColored: " << nColored << std::endl;
    // }

    // void elkin() {
    //     // 4. Run algo for sparse Vertices [12: Elkin]
    //     //all_nodes - graphToDense, sparse_value = cds.invalidNode
    //     print_nColored(); // debug
	// std::cout << "Elkin:" << std::endl;
    //     coloring_elkin_subalgo_interface(g, coloring, graphToDense, invalidNode, algoParams);
    //     print_nColored(); // debug
	// detailTimer.endPhase("elkin");
    // }
    void barenboim() {
        // 5. Run algo for residual graph [9: Barenboim]
	std::cout << "Barenboim:" << std::endl;
	coloring_barenboim_subalgo_interface(g, coloring);
	// print_nColored(); // debug
	// detailTimer.endPhase("barenboim");
    }
};

template <class CGraph>
int graph_coloring_dense_sparse(CGraph& g, std::vector<ColorID> &coloring) {
    // A fundamental assumption of the algo is that the leader of a dense component
    // can color the members in sequence
    // this seems to assume the dense coloring steps of a single compnent might not be parallelizable
    // So we either need many dense components or need them to be very small
    // should they be large and few, then either the graph is not very densely connected
    // or the whole graph is densely connected. But then it is hard to have a faster than linear algo anyway.

    Coloring_Dense_Sparse cds(g,coloring);

    cds.decomposition();
    cds.initial_coloring();
    cds.coloring_steps();

    // relevant for next steps:
    // coloring - 0 if not yet colored
    // cds.graphToDense - if cds.invalidNode, then sparse
    // possibly: also send palettes for sparse vertices, do have them

    cds.barenboim();

    return cds.n_colors();
}

} // namespace GMS::Coloring

#endif // COLORING_DENSE_SPARSE_H_
