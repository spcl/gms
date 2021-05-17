# Preprocessing
This folder contains algorithms used for preprocessing of graphs, and a benchmark runner which evaluates the performance of the individual implementations.

## Algorithms
Several versions of vertex ordering algorithms are implemented:

[TODO]: <> (We should directly link this list with the names of the algorithms in question.)

- **Degeneracy ordering**
  - Matula et al. variation (Sequential and Parallel version)
  - Fast Approximation (Based on Pseudocode from Grzegorz Kwasniewski)
    - Parallelization is not yet satisfactory due to current limitations if powerset
- **Degree ordering**
  - Straightforward parallel implementation

## Structure
The header file preprocessing.h includes all relevant files, and can be used from other modules with:
```cpp
#include <gms/algorithms/preprocessing/preprocessing.h>
```

Implementations are contained in the namespaces `PpSequential` and `PpParallel` depending on whether they use parallelization.
The namespace `PpVerifier` contains references to the verification and evaluation functions.

## Ordering functions
Ordering functions generally have a signature where

```cpp
template <class SGraph, class Output = pvector<NodeId>>
void ordering(const SGraph &graph, Output &output)
```

[TODO]: <> ("Direction" of the ordering)

The output can also be a `std::vector<NodeId>` or a different container with similar interface, and will contain the computed ordering of each vertex.

[TODO]: <> (And currently they also return a vector by value)
Also notably, `PpParallel::getDegeneracyOrderingApproxCGraph` and `PpParallel::getDegeneracyOrderingApproxSGraph` take an additional `epsilon` parameter which can be bound with a `std::bind`. 

[TODO?]: <> (```
PreprocLib
│   README.md
│   CMakeLists.txt    
│   general.h
|   preprocessing.cpp
│   preprocessing.h
|
└───parallel
│   │   <Implementation files as headers>
|
└───sequential
│   │   <Implementation files as headers>
|
└───verifiers
│   │   <Implementation files as headers>
```)

## Using the ordering functions
[TODO]: <> (Maybe this can be updated in the future or moved to a more appropriate place)

To see how ordering functions can be used together with other benchmark functions, see
- `gms/algorithms/set_based/max_clique/bron_kerbosch.cc` for how to integrate it with `BenchmarkBk` and similar benchmark runner functions.
- `gms/algorithms/non_set_based/clique_counting/clique_count_danisch_edgeParallel.cc` for how to integrate it with the benchmark `Pipeline` class.

[TODO]: <> (Also mention coloring once it's back in the repo as an exapmle on how to use this explicitly.)

## Guidelines
There are no enforced guidelines. The described structure as well as listed guidelines defines a standard which should help the collaboration. It may be changed, extended or adapted in any other way by any collaborator but fundamental change intents should be communicated.

- Only include the most relevant functionality in the `PpSequential` and `PpParallel` namespaces.
  - You can hide other code in sub namespaces of the corresponding namespaces.
- Include only libraries in `general.h` which are relevant for all implementation files
- Use the executable of `preprocessing.cc` for benchmarking and comparison with other implementations/variations
- Unit tests should be included directly inside the tests folder of `minebench`
- `PreprocLib` should not contain any files which will be compiled to executables, i.e. it is a header-only library.