# Testing
This directory contains various unittests.

In general they aren't supposed to test each and every functionality, but to be helpful when debugging something.
Note that many of these unittests don't test all variants yet and will need to be refactored yet.

Finished test cases:
- `sets.cpp`: Can be used to test new set implementations.
- `set_graph.cpp`: Used to test SetGraph constructors.

To compile a test called `x.cpp` run `make x_test`.

## Bron-Kerbosch tests
The test harness consists of fixtures and parameterized test cases.  
The aim of the test suite is mainly regression testing on smaller graphs.
One can always compare the results of arbitrary sized graphs in the benchmark using the verification flag.  
The fixture contains following graphs:
- predefined graphs of size 3
- predefined graphs in the testGraphs folder
- random graphs of size 10, 50 and 100

The unit tests are parameterized with specified algorithms.
Thus, each algorithm runs once on each graph.
  
Following checks are made:
1. Check if the resulting set contains only cliques
2. Check if the resulting sets cliques are maximal
3. Compare solution of one algorithm with an other 

A base algorithm is specified beforehand (by default `BkSimpel::mce`).
The base is first rigorously tested against all testgraphs.
Then the rest of the algorithms are compared against the base, whereas the big graph is disabled by default in order to reduce testing time.
The tests use a Timeout which can be set on a test case basis.

TODO: was this moved to the preprocessing test?  
Degeneracy ordering algorithms are tested similar to mce.

## Coders tests
TODO

## Clique counting
TODO