#include "test_helper.h"

#include <unordered_set>

#include <gms/representations/sets/sorted_set.h>
#include <gms/algorithms/set_based/maximal_clique_enum/bron_kerbosch.h>
#include <gms/algorithms/preprocessing/preprocessing.h>

#include <iostream>
#include <fstream>

#include <set>
#include <map>
#include <string>
#include <numeric>
#include <random>
#include <future> //For Timeout Tests

using testing::ElementsAre;
using testing::ElementsAreArray;
using testing::IsSubsetOf;
using testing::IsSupersetOf;
using testing::Not;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

typedef std::vector<RoaringSet> (*MceFunc)(const CSRGraph &graph);      //typedef for mce function
typedef void (*DegOrderFunc)(const RoaringGraph &graph, std::vector<NodeId> &output); //typedef for degOrder function

#define TEST_TIMEOUT_BEGIN                             \
    std::promise<bool> promisedFinished;               \
    auto futureResult = promisedFinished.get_future(); \
                              std::thread([&](std::promise<bool>& finished) {

#define TEST_TIMEOUT_FAIL_END(X)             \
    finished.set_value(true);                \
    }, std::ref(promisedFinished)).detach(); \
    EXPECT_TRUE(futureResult.wait_for(std::chrono::milliseconds(X)) != std::future_status::timeout) << "TIMEOUT of " << X << " ms was EXCEEDED!!!!!!!";

#define TEST_TIMEOUT_SUCCESS_END(X)          \
    finished.set_value(true);                \
    }, std::ref(promisedFinished)).detach(); \
    EXPECT_FALSE(futureResult.wait_for(std::chrono::milliseconds(X)) != std::future_status::timeout);

//---------------------------Tests Helper-------------------------------------

//HACK: I will create a set out of roaringSet, because DISABLE_COPY makes Life hard
//REALLY REALLY BAD AND UNEFFICIENT!!
std::set<NodeId> roaringToSet(RoaringSet &roaring)
{
    std::set<NodeId> res;
    for (const auto &v : roaring)
    {
        res.insert(v);
    }
    return res;
}

//HACK: Same thing
std::vector<std::set<NodeId>> roaringSetstoVecOfSets(std::vector<RoaringSet> &setOfRoaringSets)
{
    std::vector<std::set<NodeId>> res;
    for (auto &r : setOfRoaringSets)
    {
        res.push_back(roaringToSet(r));
    }
    return res;
}

void checkIsAClique(RoaringGraph &g, RoaringSet &set)
{
    auto setCasted = roaringToSet(set);
    for (const auto &v : set)
    {
        RoaringSet neighs = g.out_neigh(v).clone();
        neighs.union_inplace(v);
        ASSERT_THAT(roaringToSet(neighs), IsSupersetOf(setCasted)) << "=============> The given set is not a clique!";
    }
}

void checkCliqueIsMaximal(const RoaringGraph &g, const RoaringSet &set)
{
    //For all nodes in univ
    //auto setCasted = roaringToSet(set);
    for (NodeId v = 0; v < g.num_nodes(); v++)
    {
        if (!set.contains(v)) //If the node is not in the clique
        {
            //To show: it exists u in set, s.t. u NOT in neighs(v)
            //We show, that set-neighs != {}
            const RoaringSet &neighs = g.out_neigh(v);
            ASSERT_FALSE(set.difference(neighs).cardinality() == 0) << "=============> Clique is not Maximal";
        }
    }
}

void checkMCE(MceFunc mce, CSRGraph *graph)
{
    RoaringGraph roaringGraph = RoaringGraph::FromCGraph(*graph);
    auto solution = mce(*graph);
    for (auto &clique : solution)
    {
        ASSERT_NO_FATAL_FAILURE(checkIsAClique(roaringGraph, clique));
        ASSERT_NO_FATAL_FAILURE(checkCliqueIsMaximal(roaringGraph, clique));
    }
}

void checkMCE(MceFunc mce, std::set<CSRGraph **> &graphWrapper)
{
    for (const auto &graph : graphWrapper)
    {
        checkMCE(mce, *graph);
    }
}

//Compare two functions
void compareMCEs(MceFunc mce, MceFunc mceSol, CSRGraph *graph)
{
    if (mce == mceSol)
        return;

    auto actual = mce(*graph);
    auto actualSet = roaringSetstoVecOfSets(actual);
    auto expected = mceSol(*graph);
    auto expectedSet = roaringSetstoVecOfSets(expected);
    ASSERT_THAT(actualSet, UnorderedElementsAreArray(expectedSet));
}

void checkDegOrder(DegOrderFunc degOrder, CSRGraph *graph)
{
    const RoaringGraph rgraph = RoaringGraph::FromCGraph(*graph);
    auto degeneracy = DegeneracyOrderingVerifier::getDegeneracy(RoaringGraph::FromCGraph(*graph));
    std::vector<NodeId> order;
    degOrder(RoaringGraph::FromCGraph(*graph), order);
    RoaringSet visited = {};
    for (auto &v : order)
    {
        auto deg = rgraph.out_neigh(v).difference(visited).cardinality();
        ASSERT_GE(degeneracy, deg) << "Not a degeneray Ordering, failed on: " << v;
        visited.union_inplace(v);
    }
}

template <size_t size_n, size_t size_m>
std::vector<int> degsFromMatrix(bool (&arr)[size_n][size_m])
{
    std::vector<int> degs(size_n);
    for (unsigned int i = 0; i < size_n; i++)
    {
        for (unsigned int j = 0; j < size_m; j++)
        {
            if (arr[i][j])
                degs[i]++;
        }
    }
    return degs;
}

template <size_t size_n, size_t size_m>
CSRGraph *csrFromMatrix(bool (&arr)[size_n][size_m])
{
    std::vector<int> degs = degsFromMatrix(arr);
    int sumDeg = std::accumulate(degs.begin(), degs.end(), 0);

    NodeId *neighs = new NodeId[sumDeg];
    NodeId **index = new NodeId *[size_n + 1];

    //Fill neighs
    int pos = 0;
    for (unsigned int i = 0; i < size_n; i++)
    {
        for (unsigned int j = 0; j < size_m; j++)
        {
            if (arr[i][j])
            {
                neighs[pos] = j;
                pos++;
            }
        }
    }

    //Fill index
    int offset = 0;
    for (unsigned int i = 0; i < size_n; i++)
    {
        index[i] = neighs + offset;
        offset += degs[i];
    }
    index[size_n] = neighs + sumDeg; // last entry points to the end of neighs

    CSRGraph *res = new CSRGraph(size_n, index, neighs);
    return res;
}

//---------------------------END Tests Helper-------------------------------------

//---------------------------Test Fixture-----------------------------------------

//Base Graph Tests
template <class T>
class GraphFixtureBase : public T
{
private:
    std::default_random_engine gen;
    std::uniform_real_distribution<float> distribution;

    template <unsigned int nodeCount>
    void generateRandomGraph(bool (&arr)[nodeCount][nodeCount], float edgeProb)
    {
        for (unsigned int v = 0; v < nodeCount; v++)
        {
            arr[v][v] = false;
            for (unsigned int u = v + 1; u < nodeCount; u++)
            {
                if (bernoulli(edgeProb))
                {
                    arr[v][u] = true;
                    arr[u][v] = true;
                }
                else
                {
                    arr[v][u] = false;
                    arr[u][v] = false;
                }
            }
        }
    }

    bool bernoulli(float prob)
    {
        return distribution(gen) < prob;
    }

protected:
    CSRGraph *graphBASIC1;
    CSRGraph *graphBASIC2;
    CSRGraph *graphRandSmall;
    CSRGraph *graphRandMedium;
    CSRGraph *graphRandBig;
    std::set<CSRGraph **> wrapperBasic{&graphBASIC1, &graphBASIC2};
    std::set<CSRGraph **> wrapperRest{&graphRandSmall, &graphRandMedium, &graphRandBig};

    virtual void SetUp()
    {
        //Predefined Graphs
        bool graphMatrix_BASIC[3][3] = {{0, 0, 1}, {0, 0, 1}, {1, 1, 0}};

        bool graphMatrix1[3][3] = {
            {0, 1, 1},
            {1, 0, 1},
            {1, 1, 0}};
        graphBASIC1 = csrFromMatrix(graphMatrix_BASIC);
        graphBASIC2 = csrFromMatrix(graphMatrix1);

        //Random Graphs (To be determinsitc, comment out next two lines)
        std::random_device rd;                                          //Will be used to obtain a seed for the random number engine
        gen = std::default_random_engine(rd());                         //take the seed
        distribution = std::uniform_real_distribution<float>(0.0, 1.0); //Set the distribution to be U(0,1)

        bool small[10][10];
        generateRandomGraph(small, 0.5);
        graphRandSmall = csrFromMatrix(small);
        bool medium[50][50];
        generateRandomGraph(medium, 0.5);
        graphRandMedium = csrFromMatrix(medium);
        bool big[100][100];
        generateRandomGraph(big, 0.5);
        graphRandBig = csrFromMatrix(big);
    }

    virtual void TearDown()
    {
        for (auto g : wrapperBasic)
            delete *g;
        for (auto g : wrapperRest)
            delete *g;
    }
};

//For non parameterized Tests
class GraphFixtureTest : public GraphFixtureBase<testing::Test>
{
};

class ParamFixtureMCETest : public GraphFixtureBase<
                                testing::TestWithParam<MceFunc>>
{
};

class ParamFixtureDEGTest : public GraphFixtureBase<
                                testing::TestWithParam<DegOrderFunc>>
{
};

//---------------------------End Test Fixture-------------------------------------

//---------------------------Tests-----------------------------------------------

TEST(TestMacro, TestDefinition)
{
//Test if TestMacro is defined
#ifdef MINEBENCH_TEST
    if (BkHelper::checkDefTestMacro())
        SUCCEED();
    else
        FAIL() << "Macro MINEBENCH_TEST is not defined in BkLib!";
#else
    FAIL() << "Macro MINEBENCH_TEST is not defined in Testclass!";
#endif
}

template <class SGraph = RoaringGraph>
std::string PrintMceFunc(testing::TestParamInfo<MceFunc> mceInfo)
{
    auto mce = mceInfo.param;
    if (mce == BkSequential::BkSimpleMce<SGraph>)
        return "BronKerbosch_Simple";
    else if (mce == BkSequential::BkTomitaMce<SGraph>)
        return "BronKerbosch_Tomita";
    else if (mce == BkSequential::BkEppsteinMce<SGraph>)
        return "BronKerbosch_Eppstein";
    //else if (mce == BkParallel::BkEppsteinDegeneracy<SGraph>)
    //    return "BronKerbosch_Eppstein_PAR";
    else
        return "UNKNOWN Function";
}

std::string PrintDegOrderFunc(testing::TestParamInfo<DegOrderFunc> degOrderInfo)
{
    auto deg = degOrderInfo.param;

    if (deg == &PpSequential::getDegeneracyOrderingMatula<RoaringGraph, false, std::vector<NodeId>>)
        return "getDegeneracyOrdering";
    // else if (deg == PpSequential::getDegeneracyOrderingMatula)
    //     return "getDegeneracyOrderingMatula";
    // else if (deg == PpParallel::getDegeneracyOrderingMatula)
    //     return "getDegeneracyOrderingMatula PAR";
    else
        return "UNKNOWN Function";
}

std::string getScopedTrace(MceFunc curFunction, std::string test)
{
    testing::TestParamInfo<MceFunc> mceInfo(curFunction, 1);
    return PrintMceFunc(mceInfo).append("_").append(test);
}

std::string getScopedTrace(DegOrderFunc curFunction, std::string test)
{
    testing::TestParamInfo<DegOrderFunc> degOrderInfo(curFunction, 1);
    return PrintDegOrderFunc(degOrderInfo).append("_").append(test);
}



INSTANTIATE_TEST_SUITE_P(
    MCETests,
    ParamFixtureMCETest,
    testing::ValuesIn({
            BkSequential::BkSimpleMce<RoaringGraph>,
            BkSequential::BkTomitaMce<RoaringGraph>,
            BkSequential::BkEppsteinMce<RoaringGraph>
            //BkParallel::BkEppsteinDegree<RoaringGraph>,
    }),
    PrintMceFunc);

INSTANTIATE_TEST_SUITE_P(
    DegOrderTests,
    ParamFixtureDEGTest,
    ::testing::Values(PpSequential::getDegeneracyOrderingMatula<RoaringGraph, false, std::vector<NodeId>>),
    PrintDegOrderFunc);

//mceBase is used to compare all other versions against
MceFunc mceBase = BkSequential::BkSimpleMce<RoaringGraph>;

//Test the BasicrecordSimple
TEST_F(GraphFixtureTest, TestBaseRigorously)
{
    TEST_TIMEOUT_BEGIN
    SCOPED_TRACE("BASIC");
    ASSERT_NO_FATAL_FAILURE(checkMCE(mceBase, wrapperBasic));
    SCOPED_TRACE("SMALL");
    ASSERT_NO_FATAL_FAILURE(checkMCE(mceBase, graphRandSmall));
    SCOPED_TRACE("MEDIUM");
    ASSERT_NO_FATAL_FAILURE(checkMCE(mceBase, graphRandMedium));
    SCOPED_TRACE("BIG");
    ASSERT_NO_FATAL_FAILURE(checkMCE(mceBase, graphRandBig));
    TEST_TIMEOUT_FAIL_END(50000)
}

// TEST(PrefixSum, Simple)
// {
//     int keep[] = {1, 2, 3, 4, 5};
//     int psum[5];

//     DegOrderApproxPAR::prefixSum(keep, psum, 5);
//     ASSERT_THAT(psum, ElementsAre(1, 3, 6, 10, 15));

//     for (int N : {10, 15, 20, 22, 55, 67, 98, 100, 1000, 9999, 99999})
//     {
//         int *keep2 = new int[N];
//         int *psum2 = new int[N];
//         int *res2 = new int[N];
//         std::iota(keep2, keep2 + N, 0);

//         DegOrderApproxPAR::prefixSum(keep2, psum2, N);
//         std::partial_sum(keep2, keep2 + N, res2);

//         ASSERT_THAT(std::vector<int>(keep2, keep2 + sizeof keep2 / sizeof keep2[0]), ElementsAreArray(std::vector<int>(res2, res2 + sizeof res2 / sizeof res2[0])));
//         delete[] keep2;
//         delete[] psum2;
//         delete[] res2;
//     }
// }

TEST_P(ParamFixtureMCETest, bronKerboschBASIC)
{
    TEST_TIMEOUT_BEGIN
    SCOPED_TRACE("BASIC");
    ASSERT_NO_FATAL_FAILURE(checkMCE(GetParam(), wrapperBasic));
    TEST_TIMEOUT_FAIL_END(1000)
}

TEST_P(ParamFixtureMCETest, bronKerboschRandSMALL)
{
    TEST_TIMEOUT_BEGIN
    SCOPED_TRACE(getScopedTrace(GetParam(), "SMALL"));
    ASSERT_NO_FATAL_FAILURE(compareMCEs(GetParam(), mceBase, graphRandSmall));
    TEST_TIMEOUT_FAIL_END(5000)
}

TEST_P(ParamFixtureMCETest, bronKerboschRandMEDIUM)
{
    TEST_TIMEOUT_BEGIN
    SCOPED_TRACE(getScopedTrace(GetParam(), "MEDIUM"));
    ASSERT_NO_FATAL_FAILURE(compareMCEs(GetParam(), mceBase, graphRandMedium));
    TEST_TIMEOUT_FAIL_END(10000)
}

TEST_P(ParamFixtureMCETest, bronKerboschRandBIG)
{
    TEST_TIMEOUT_BEGIN
    SCOPED_TRACE(getScopedTrace(GetParam(), "BIG"));
    ASSERT_NO_FATAL_FAILURE(compareMCEs(GetParam(), mceBase, graphRandBig));
    TEST_TIMEOUT_FAIL_END(80000)
}

TEST_P(ParamFixtureDEGTest, DegeneracyOrdering)
{
    TEST_TIMEOUT_BEGIN
    SCOPED_TRACE(getScopedTrace(GetParam(), "SMALL"));
    ASSERT_NO_FATAL_FAILURE(checkDegOrder(GetParam(), graphRandSmall));
    SCOPED_TRACE(getScopedTrace(GetParam(), "MEDIUM"));
    ASSERT_NO_FATAL_FAILURE(checkDegOrder(GetParam(), graphRandMedium));
    SCOPED_TRACE(getScopedTrace(GetParam(), "BIG"));
    ASSERT_NO_FATAL_FAILURE(checkDegOrder(GetParam(), graphRandBig));
    TEST_TIMEOUT_FAIL_END(10000)
}

TEST(DegeneracyOrdering, Example)
{
    TEST_TIMEOUT_BEGIN
    CSRGraph graph = loadGraphFromFile("eppsteinExample.el");
    graph.PrintStats();
    graph.PrintTopology();

    int degeneracy = DegeneracyOrderingVerifier::getDegeneracy(RoaringGraph::FromCGraph(graph));
    std::cout << "Expected Degeneracy is: " << degeneracy << std::endl;

    // auto degOrder = PpSequential::getDegeneracyOrderingMatula(RoaringGraph(graph));
    // BkHelper::printSet("DegOrder", degOrder);

    // ASSERT_NO_FATAL_FAILURE(checkDegOrder(PpSequential::getDegeneracyOrderingMatula, &graph));
    TEST_TIMEOUT_FAIL_END(5000)
}

TEST(Eppstein, DISABLED_example)
{
    TEST_TIMEOUT_BEGIN
    CSRGraph graph = loadGraphFromFile("eppsteinExample.el");
    ASSERT_NO_FATAL_FAILURE(compareMCEs(BkEppstein::mce<RoaringGraph>, BkSimple::mce<RoaringGraph>, &graph));
    TEST_TIMEOUT_FAIL_END(5000)
}

TEST(Tomita, DISABLED_Example)
{
    TEST_TIMEOUT_BEGIN
    CSRGraph graphTomita = loadGraphFromFile("tomitaExample.el");
    auto actual = BkTomita::mce<RoaringGraph>(graphTomita);
    auto expected = BkSimple::mce<RoaringGraph>(graphTomita);
    BkHelper::printSetOfSet(expected);
    BkHelper::printSetOfSet(actual);
    ASSERT_TRUE(false);
    TEST_TIMEOUT_FAIL_END(5000)
}

//---------------------------END Tests-----------------------------------------------