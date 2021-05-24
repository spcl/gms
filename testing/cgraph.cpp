#include "test_helper.h"

template <class TSet>
class CGraphTest : public testing::Test
{};

using testing::UnorderedElementsAre;

// TODO compile test for 32 and 64 bit

using Implementations =
testing::Types<
        CSRGraph,
        Kbit_Adjacency_Array,
        Kbit_Adjacency_Array_Local,
        VarintByteBasedGraph,
        VarintWordBasedGraph
>;

TYPED_TEST_SUITE(CGraphTest, Implementations);

template <class CGraph>
CGraph load_CGraph(int)
{
    CSRGraph g;

    // TODO FIXME with symmetrize=true everything fails!
    //g = loadGraphFromFile("smallRandom1.el", false);

    // TODO test with a larger graph
    g = loadGraphFromFile("micro.el");
    CLBase cli(0, {}, "dummy");
    Builder builder(cli);

    // TODO refactoring
    if constexpr (std::is_same_v<CGraph, CSRGraph>) {
//Ignore Compiler-Warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
        return std::move(g);
#pragma GCC diagnostic pop
    } else {
        return builder.csrToCGraphGeneric<CGraph>(g);
    }
}

TYPED_TEST(CGraphTest, LoadGraph)
{
    ASSERT_NO_THROW(load_CGraph<TypeParam>(0));
}

TYPED_TEST(CGraphTest, NumNodes)
{
    auto g = load_CGraph<TypeParam>(0);
    ASSERT_EQ(g.num_nodes(), 2);
}

TYPED_TEST(CGraphTest, NumEdges)
{
    auto g = load_CGraph<TypeParam>(0);
    ASSERT_EQ(g.num_edges(), 1);
}

// TODO this isn't ideal yet either (interplay with symmetrize)
TYPED_TEST(CGraphTest, IsDirected)
{
    auto g = load_CGraph<TypeParam>(0);
    ASSERT_EQ(g.directed(), false);
}

TYPED_TEST(CGraphTest, Degree)
{
    auto g = load_CGraph<TypeParam>(0);
    ASSERT_EQ(g.out_degree(0), 1);
}

TYPED_TEST(CGraphTest, Neighborhood)
{
    auto g = load_CGraph<TypeParam>(0);
    std::vector<NodeId> neigh;
    for (NodeId v : g.out_neigh(0)) {
        neigh.push_back(v);
    }
    ASSERT_THAT(neigh, UnorderedElementsAre(1));
    neigh.clear();

    for (NodeId v : g.out_neigh(1)) {
        neigh.push_back(v);
    }
    ASSERT_THAT(neigh, UnorderedElementsAre(0));
}
