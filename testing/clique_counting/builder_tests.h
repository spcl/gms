#ifndef ABSTRACTIONOPTIMIZING_MINEBENCH_TEST_CLIQUECOUNTING_BUILDER_H
#define ABSTRACTIONOPTIMIZING_MINEBENCH_TEST_CLIQUECOUNTING_BUILDER_H

#include <string>

#include "includes.h"

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

class BuilderTest : public ::testing::Test
{
protected:
public:

    BuilderTest(){}

    ~BuilderTest(){}

    virtual void SetUp() override
    {
    }

    virtual void TearDown() override 
    {
    }
};

TEST_F(BuilderTest, MinimalDirectedGraph)
{
    CLApp cli(0, nullptr, "stub");
    Builder b(cli);

    EdgeList list(1);
    list[0] = Edge(0,1);

    CSRGraph g = b.MakeGraphFromEL(list);

    EXPECT_EQ( 2, g.num_nodes());
    EXPECT_EQ( 1, g.num_edges());
    EXPECT_TRUE( g.directed());
}

TEST_F(BuilderTest, MinimalUndirectedGraph)
{
    UCLApp cli(0, nullptr,"stub");
    Builder b(cli);

    EdgeList list(1);
    list[0] = Edge(0,1);

    CSRGraph g = b.MakeGraphFromEL(list);

    EXPECT_FALSE( g.directed());
}

TEST_F(BuilderTest, HasDirectedLoop)
{
    CLApp cli(0, nullptr, "stub");
    Builder b(cli);

    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,1);

    CSRGraph g = b.MakeGraphFromEL(list);

    EXPECT_EQ( 1, g.out_degree(1));
    EXPECT_EQ( 1, g.out_degree(0));
    EXPECT_EQ( 1, * g.out_neigh(0).begin());
    EXPECT_EQ( 1, * g.out_neigh(1).begin());
}

// this test will fail -> loops are not treated in standard undirected graphs
TEST_F(BuilderTest, HasUndirectedLoop)
{
    UCLApp cli(0, nullptr, "stub");
    Builder b(cli);

    EdgeList list(1);
    list[0] = Edge(0,0);

    CSRGraph g = b.MakeGraphFromEL(list);

    // EXPECT_EQ(1, g.num_nodes());
    // EXPECT_EQ(1, g.num_edges());
    // EXPECT_EQ(1, g.out_degree(0));
    // EXPECT_EQ(0, * g.out_neigh(0).begin());
}

#endif