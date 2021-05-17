#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_SUBGRAPHBUILDER
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_SUBGRAPHBUILDER

#include <iostream>
#include "includes.h"

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

class SubGraphBuilderFixture : public ::testing::Test
{
    UCLApp ucl = UCLApp(0, nullptr, "undir");
    CLApp dcl = CLApp(0, nullptr, "dir");

protected:
    Builder UndirB() {return Builder(ucl);}
    Builder DirB() {return Builder(dcl);}

public: 
    SubGraphBuilderFixture() {}
    ~SubGraphBuilderFixture(){}

    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
    }
};

TEST_F(SubGraphBuilderFixture, CreatesSmallSubGraph)
{
    EdgeList list(4);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(1,3);
    list[3] = Edge(2,3);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilder(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(0, subG.num_edges());

    EXPECT_EQ(0, subG.out_degree(mapper.NewIndex(1)));
    EXPECT_EQ(0, subG.out_degree(mapper.NewIndex(2)));
}

TEST_F(SubGraphBuilderFixture, CreatesSmallSubGraphWithInverse)
{
    EdgeList list(4);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(1,3);
    list[3] = Edge(2,3);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilderWInverse(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(0, subG.num_edges());

    EXPECT_EQ(0, subG.out_degree(mapper.NewIndex(1)));
    EXPECT_EQ(0, subG.out_degree(mapper.NewIndex(2)));

    EXPECT_EQ(0, subG.in_degree(mapper.NewIndex(1)));
    EXPECT_EQ(0, subG.in_degree(mapper.NewIndex(2)));
}

TEST_F(SubGraphBuilderFixture, CreatesLargerSubGraph)
{
    EdgeList list(5);
    list[0] = Edge(0,2);
    list[1] = Edge(0,3);
    list[2] = Edge(1,0);
    list[3] = Edge(2,1);
    list[4] = Edge(2,3);
    
    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilder(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(1, subG.num_edges());
    EXPECT_EQ(1, subG.out_degree(mapper.NewIndex(2)));
    EXPECT_EQ(0, subG.out_degree(mapper.NewIndex(3)));
}

TEST_F(SubGraphBuilderFixture, CreatesLargerSubGraphWithInverse)
{
    EdgeList list(5);
    list[0] = Edge(0,2);
    list[1] = Edge(0,3);
    list[2] = Edge(1,0);
    list[3] = Edge(2,1);
    list[4] = Edge(2,3);
    
    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilderWInverse(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(1, subG.num_edges());
    EXPECT_EQ(1, subG.out_degree(mapper.NewIndex(2)));
    EXPECT_EQ(0, subG.out_degree(mapper.NewIndex(3)));

    EXPECT_EQ(1, subG.in_degree(mapper.NewIndex(3)));
}

TEST_F(SubGraphBuilderFixture, SmallGraphFromEdge)
{
    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(1,2);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    EXPECT_TRUE(g.directed());
    auto builder = Builders::SubGraphBuilder(g, 3);

    cc::Graph_T subG = builder.buildSubGraph(0,1);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(1, subG.num_nodes());
    EXPECT_EQ(0, subG.num_edges());
    EXPECT_EQ(0, mapper.NewIndex(2));
}

TEST_F(SubGraphBuilderFixture, SmallGraphFromEdgeWithInverse)
{
    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(1,2);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    EXPECT_TRUE(g.directed());
    auto builder = Builders::SubGraphBuilderWInverse(g, 3);

    cc::Graph_T subG = builder.buildSubGraph(0,1);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(1, subG.num_nodes());
    EXPECT_EQ(0, subG.num_edges());
    EXPECT_EQ(0, mapper.NewIndex(2));
}

TEST_F(SubGraphBuilderFixture, SmallishGraphFromEdge)
{
    EdgeList list(6);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(0,3);
    list[3] = Edge(1,2);
    list[4] = Edge(1,3);
    list[5] = Edge(2,3);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    EXPECT_TRUE(g.directed());
    auto builder = Builders::SubGraphBuilder(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0,1);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(1, subG.num_edges());
    EXPECT_EQ(1, subG.out_degree(mapper.NewIndex(2)));
}

TEST_F(SubGraphBuilderFixture, SmallishGraphFromEdgeWithInverse)
{
    EdgeList list(6);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(0,3);
    list[3] = Edge(1,2);
    list[4] = Edge(1,3);
    list[5] = Edge(2,3);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    EXPECT_TRUE(g.directed());
    auto builder = Builders::SubGraphBuilderWInverse(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0,1);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(1, subG.num_edges());
    EXPECT_EQ(1, subG.out_degree(mapper.NewIndex(2)));

    EXPECT_EQ(1, subG.in_degree(mapper.NewIndex(3)));
}

TEST_F(SubGraphBuilderFixture, LargerGraphFromEdge)
{
    EdgeList list(8);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(0,3);
    list[3] = Edge(1,2);
    list[4] = Edge(1,3);
    list[5] = Edge(1,4);
    list[6] = Edge(3,2);
    list[7] = Edge(3,4);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilder(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0,1);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(1, subG.num_edges());
    EXPECT_EQ(1, subG.out_degree(mapper.NewIndex(3)));
}


TEST_F(SubGraphBuilderFixture, LargerGraphFromEdgeWithInverse)
{
    EdgeList list(8);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(0,3);
    list[3] = Edge(1,2);
    list[4] = Edge(1,3);
    list[5] = Edge(1,4);
    list[6] = Edge(3,2);
    list[7] = Edge(3,4);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilderWInverse(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0,1);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(2, subG.num_nodes());
    EXPECT_EQ(1, subG.num_edges());
    EXPECT_EQ(1, subG.out_degree(mapper.NewIndex(3)));

    EXPECT_EQ(1, subG.in_degree(mapper.NewIndex(2)));
}

TEST_F(SubGraphBuilderFixture, Get4CliqueWithInverse)
{
    EdgeList list(10);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(1,2);
    list[5] = Edge(1,3);
    list[6] = Edge(1,4);
    list[7] = Edge(2,3);
    list[8] = Edge(2,4);
    list[9] = Edge(3,4);

    cc::Graph_T g = DirB().MakeGraphFromEL(list);
    auto builder = Builders::SubGraphBuilderWInverse(g, 4);

    cc::Graph_T subG = builder.buildSubGraph(0);
    auto mapper = builder.GetMapping();

    EXPECT_EQ(4, subG.num_nodes());
    EXPECT_EQ(0, subG.in_degree(mapper.NewIndex(1)));
    EXPECT_EQ(1, subG.in_degree(mapper.NewIndex(2)));
    EXPECT_EQ(2, subG.in_degree(mapper.NewIndex(3)));
    EXPECT_EQ(3, subG.in_degree(mapper.NewIndex(4)));
}

#endif