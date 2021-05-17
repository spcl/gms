#ifndef OPTIONS_H_
#define OPTIONS_H_

// options for Log(Graph)
// SIMPLE_GAP_ENCODING: will store differences between vertex_IDs
#ifndef SIMPLE_GAP_ENCODING
	#define SIMPLE_GAP_ENCODING 0
#endif
// LOCAL_APPROACH: 0: global approach, 1: local approach
#ifndef LOCAL_APPROACH  // should be defined in the makefile
	#define LOCAL_APPROACH 0
#endif

#ifndef BIT_TREE
	#define BIT_TREE 0
#endif

#if BIT_TREE
	#define ITERATE_NEIGHBOURHOOD(ITERATOR, VERTEX, CODE)     \
		if(g.encoding(VERTEX))                                \
			for (NodeId ITERATOR: g.bit_tree_neigh(VERTEX)) { \
				CODE                                          \
			}                                                 \
		else                                                  \
			for (NodeId ITERATOR: g.out_neigh(VERTEX)) {      \
				CODE                                          \
			}

	#define ITERATE_NEIGHBOURHOOD_2(ITERATOR, VERTEX, CODE)   \
		if(g.encoding(VERTEX)) {                              \
			auto ITERATOR = g.out_neigh(VERTEX).begin();      \
			CODE                                              \
		}                                                     \
		else  {                                               \
			auto ITERATOR = g.out_neigh(VERTEX).begin();      \
			CODE                                              \
		}
#else
	#define ITERATE_NEIGHBOURHOOD(ITERATOR, VERTEX, CODE)     \
		for (NodeId ITERATOR : g.out_neigh(VERTEX)) {         \
				CODE                                          \
			}

	#define ITERATE_NEIGHBOURHOOD_2(ITERATOR, VERTEX, CODE)   \
			auto ITERATOR = g.out_neigh(VERTEX).begin();      \
				CODE
#endif

typedef int32_t NodeId;
typedef int32_t WeightT;

// typedef std::iterator<
//                     std::input_iterator_tag, // iterator_category
//                     NodeId,                  // value_type
//                     NodeId,                  // difference_type
//                     const NodeId*,           // pointer
//                     NodeId 							// reference
//                                   >
// 		NodeId_Iterator;
//
#endif // OPTIONS_H_
