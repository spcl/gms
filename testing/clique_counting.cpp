#include "gtest/gtest.h"
#include <omp.h>
#include "clique_counting/Comparer_unittest.h"
#include "clique_counting/TrackingHeap_tests.h"
#include "clique_counting/TrackingBubblingArray_tests.h"
#include "clique_counting/conversion_tests.h"
#include "clique_counting/builder_tests.h"
//#include "clique_counting/CliqueCounter_tests.h"
#include "clique_counting/CliqueCounter2_tests.h"
#include "clique_counting/SubGraphBuilder_tests.h"
//#include "clique_counting/CliqueCounterSubGraph_tests.h" <-- same as KcListing
//#include "clique_counting/CliqueCounterSubGraphPP_tests.h"
#include "clique_counting/CliqueCounterNodeParallel_tests.h"
#include "clique_counting/CliqueCounterEdgeParallel_tests.h"
//#include "clique_counting/CliqueCounterPath_tests.h"




int main(int argc, char **argv) {
  //omp_set_num_threads(1);

  printf("Running main() from %s\n", __FILE__);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}