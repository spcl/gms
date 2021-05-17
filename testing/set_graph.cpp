#include "test_helper.h"
#include <gms/representations/graphs/set_graph.h>

template <class TSet>
class SetGraphTest : public testing::Test
{};

using SetImpls =
testing::Types<
    RoaringSet,
    // NOTE: This isn't implemented at this time.
    //Roaring64Set,
    SortedSetBase<std::int32_t>,
    SortedSetBase<std::int64_t>,
    RobinHoodSetBase<std::int32_t>,
    RobinHoodSetBase<std::int64_t>
>;

TYPED_TEST_SUITE(SetGraphTest, SetImpls);

#define SGraph SetGraph<TypeParam>
#define Set TypeParam

CSRGraph BuildTestGraph(bool with_isolated) {
    GMS::CLI::Args args;
    args.symmetrize = true;
    Builder builder((GMS::CLI::GapbsCompat(args)));

    pvector<EdgePair<NodeId, NodeId>> EL;
    if (with_isolated) {
        EL.push_back(EdgePair(0, 2));
    } else {
        EL.push_back(EdgePair(0, 1));
    }

    auto g = builder.MakeGraphFromEL(EL);
    return builder.SquishGraph(g);
}

TYPED_TEST(SetGraphTest, Empty) {
    SGraph empty0(0);
    ASSERT_EQ(empty0.num_nodes(), 0);

    SGraph empty3(3);
    ASSERT_EQ(empty3.num_nodes(), 3);
    ASSERT_EQ(empty3.out_neigh(0).cardinality(), 0);
    ASSERT_EQ(empty3.out_neigh(1).cardinality(), 0);
    ASSERT_EQ(empty3.out_neigh(2).cardinality(), 0);
    ASSERT_EQ(empty3.out_degree(0), 0);
    ASSERT_EQ(empty3.out_degree(1), 0);
    ASSERT_EQ(empty3.out_degree(2), 0);
}

TYPED_TEST(SetGraphTest, FromVectors) {
    std::vector<Set> sets;
    sets.push_back(Set{1, 2});
    sets.push_back(Set{0});
    sets.push_back(Set{0, 3});
    sets.push_back(Set{2});

    SGraph g(std::move(sets));
    ASSERT_EQ(g.num_nodes(), 4);
    ASSERT_EQ(g.out_neigh(0), (Set{1, 2}));
    ASSERT_EQ(g.out_neigh(1), (Set{0}));
    ASSERT_EQ(g.out_degree(0), 2);
    ASSERT_EQ(g.out_degree(1), 1);
}

TYPED_TEST(SetGraphTest, FromCGraph_Default_WithoutIsolated) {
    auto cgraph = BuildTestGraph(false);
    SGraph g = SGraph::FromCGraph(cgraph);

    ASSERT_EQ(g.num_nodes(), 2);
    ASSERT_EQ(g.out_neigh(0), Set{1});
    ASSERT_EQ(g.out_neigh(1), Set{0});
}

TYPED_TEST(SetGraphTest, FromCGraph_Default_WithIsolated) {
    auto cgraph = BuildTestGraph(true);
    SGraph g = SGraph::FromCGraph(cgraph);

    ASSERT_EQ(g.num_nodes(), 3);
    ASSERT_EQ(g.out_neigh(0), Set{2});
    ASSERT_EQ(g.out_neigh(1), Set());
    ASSERT_EQ(g.out_neigh(2), Set{0});
}

TYPED_TEST(SetGraphTest, FromCGraph_RemoveIsolated_WithoutIsolated) {
    auto cgraph = BuildTestGraph(false);
    SGraph g = SGraph::template FromCGraph<CSRGraph, true>(cgraph);

    ASSERT_EQ(g.num_nodes(), 2);
    ASSERT_EQ(g.out_neigh(0), Set{1});
    ASSERT_EQ(g.out_neigh(1), Set{0});
}

TYPED_TEST(SetGraphTest, FromCGraph_RemoveIsolated_WithIsolated) {
    auto cgraph = BuildTestGraph(true);
    SGraph g = SGraph::template FromCGraph<CSRGraph, true>(cgraph);

    ASSERT_EQ(g.num_nodes(), 2);
    ASSERT_EQ(g.out_neigh(0), Set{1});
    ASSERT_EQ(g.out_neigh(1), Set{0});
}

TYPED_TEST(SetGraphTest, Clone) {
    auto cgraph = BuildTestGraph(false);
    SGraph g_original = SGraph::FromCGraph(cgraph);

    SGraph g = g_original.clone();

    ASSERT_EQ(g.num_nodes(), 2);
    ASSERT_EQ(g.out_neigh(0), Set{1});
    ASSERT_EQ(g.out_neigh(1), Set{0});
}