# Bron Kerbosch   
## Algorithms
This folder contains various header-only implementations of the Bron-Kerbosch algorithm.  
At the moment the following implementations are present:

[TODO]: <> (As in the preprocessing module docs, this list should be linked/expanded with the actual function names)

- Bron-Kerbosch original algorithm
- Tomita et al. pivot strategy
  - Parallel version using parallel pivot detection
- Eppstein et al. use of the degeneracy ordering, bootstrapped on Tomita.  
Parallel version using parallelism in the outer most loop 
  - Combination of parallelism in the outer loop and parallel Tomita
  - Streaming/caching scheme to reduce intersection costs in the initial load balancing function
- Eppstein et al. using Apurba et al. Load balancing strategy which iterates through the neighborhood instead of building the initial input sets using intersection. 
Parallel version using parallelism in the outer most loop
  - Starting the recursive Function called from the load balancer on subgraphs
  - Two layer expansion function: We build the subgraph at the beginning and in the second recursive call if it would reduce the size of graph by more than a give factor (e.g. `0.1`)
  - Using a Fast_Subgraph class which caches the sizes of the intersections but therefore als uses more memory
    
All versions of the last Eppstein category are superior to all the other versions.  
Be aware, that the sequential variation of Tomita (`BkTomita::expand`) wraps the pushback of a solution set in an omp critical section.
The reason for this is, that a lot of other algorithms are calling this function from a parallel context and, thus need a mutex.
The overhead in a sequential execution is very insignificant and even vanishes completely if compiled without the `-MINEBENCH_TEST` flag. The same accounts for the clique counter.

## Subproject structure
Since most used libraries are header only themselves and since I needed the code linked to both the minebench executable as well as the test executable, header only was a good choice to prevent linker issues. Simplified template handling comes as a bonus.  
```
bron_kerbosch
│   README.md
│   CMakeLists.txt    
│   general.h
│   helper.h
│   verifier.h
|   sub_graph/sub_graph.h
│   bron_kerbosch.h

└───parallel
│   │   <Implementation files as headers>
|
└───sequential
│   │   <Implementation files as headers>
```
All variations are guarded inside namespaces, thus calling the algorithms would be:  `BkSimple::mce(), BkTomita::mce(), BkEppstein::mce()`    
The corresponding test files are in the minebench test folder.
  
The library follows the **strategy design pattern**.  
Functions are templated by the algorithms they use. Following is an outdated and simplified sketch of the underlying structure
``` c++
//---------------------------------------------------------------
//File general.h
//Stores the typedef of templated functions used in the strategy pattern
typedef void (*Expand)(RoaringSet cand, RoaringSet fini, RoaringSet Q, std::vector<RoaringSet> &sol, RoaringGraph &graph);
typedef std::vector<NodeId> (*DegOrdering)(RoaringGraph graph);
typedef NodeId (*FindPivot)(RoaringGraph &graph, RoaringSet subg, RoaringSet &cand);

//---------------------------------------------------------------
//Implementations (in seperate files)
//The actual algorithms using templates
namespace BkTomitaPAR
{

template <FindPivot computePivot>
void expand(RoaringSet cand, RoaringSet fini, RoaringSet Q, std::vector<RoaringSet> &sol, RoaringGraph &graph){ /*...*/ }

template <FindPivot computePivot>
std::vector<RoaringSet> mce(const Graph &graph){ /*...*/ }
}

namespace BkEppsteinPAR
{

template <Expand expand, DegOrdering getDegeneracyOrdering>
std::vector<RoaringSet> mce(const Graph &graph){ /*...*/ }

}

//---------------------------------------------------------------
//File bron_kerbosch.h
//The Interface
namespace BkSequential
{
std::vector<RoaringSet> (&BkSimpleMce)(const Graph &graph) = BkSimple::mce;
std::vector<RoaringSet> (&BkTomitaMce)(const Graph &graph) = BkTomita::mce;
std::vector<RoaringSet> (&BkEppsteinMce)(const Graph &graph) = BkEppstein::mce;
} 

namespace BkParallel
{
std::vector<RoaringSet> (&BkTomitaPARMce)(const Graph &graph) = BkTomitaPAR::mce<BkTomitaPAR::findPivot>;
std::vector<RoaringSet> (&BkEppsteinPARMce)(const Graph &graph) = BkEppsteinPAR::mce<BkTomita::expand, PpSequential::getDegeneracyOrderingMatula>;
std::vector<RoaringSet> (&BkEppsteinDoublePARMce)(const Graph &graph) = BkEppsteinPAR::mce<BkTomitaPAR::expand<BkTomitaPAR::findPivot>, PpParallel::getDegeneracyOrderingMatula>;
} 

//---------------------------------------------------------------
//File bron_kerbosch.cc
//Using the interface
BenchmarkKernel(cli, g, BkParallel::BkEppsteinPARMce, BkVerifier::BronKerboschVerifier);
BenchmarkKernel(cli, g, BkParallel::BkEppsteinDoublePARMce, BkVerifier::BronKerboschVerifier);
BenchmarkKernel(cli, g, BkSequential::BkTomitaMce, BkVerifier::BronKerboschVerifier);

```

## Compilation
`bron_kerbosch.cc` , i.e. the benchmark driver, runs tomita and eppstein and compares -if prompted- the results with the simple algorithm. Since the output of the result is negelected in this project, it is only needed for testing. Thus, all implementations include the following:
```cpp
#ifdef MINEBENCH_TEST
        sol.push_back(std::move(R));
#endif
```
If compiled with `-MINEBENCH_TEST` flag, a result is produced. Otherwise only the algortihm execution is measured.  
- The Test-Executable compiles *with* the flag by default; the tests even fails if unset.  
- the Minebench-Executable compiles *without* the flag by default. If we want to check the  results with the provided verify function (passing `-v` as program args), we have to set it. One can use `BkHelper::checkDefTestMacro()`, which returns a bool, in order to conditon on the environment in code.

If compiled with the `-BK_COUNT` flag the algorithm counts the maximal cliques using an atomic variable.

## WIP
[TODO]: <>(This list will need to be updated)

- Execution and testing parameterized on Setcontainer (`RoaringSet`, `SortedSet`)
- Measure all components runtimes:
  - preprocessing
  - preparation before call by Loadbalancer
  - pivotfinding
  - body in one recursive call
- Measure recursion depth
  - for different preprocessing techniques
  - for different Loadbalancer
- Analyse different schedulings based on metrics
- Detect which graph metric (count of edges/vertices, average degree, corenumber) may influence recursion depth/runtimes. Maybe the right algorithm can be decided before running it.
- Analyse memory/cache behaviour using valgrind and gperftools
- Analyse Vectorization possibilities
- Compute roofline model and optimize according to intel advisor
- Try out intel icpc


