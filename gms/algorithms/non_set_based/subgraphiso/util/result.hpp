#ifndef GMB_SUBGRAPH_ISO_RESULT_H
#define GMB_SUBGRAPH_ISO_RESULT_H

#include <chrono>
#include <list>
#include <map>
#include <string>

namespace GMS::SubGraphIso
{
	// Structure to store result of our algorithms and gives timing utility functions.
	struct Result
	{
		/// The isomorphism, empty if none found.
		std::map<int, int> isomorphism;

		bool aborted = false;

		/// Total number of nodes processed.
		unsigned long long nodes = 0;

		/// A count, if enumerating.
		unsigned result_count = 0;
	};
} // namespace GMS::SubGraphIso

#endif
