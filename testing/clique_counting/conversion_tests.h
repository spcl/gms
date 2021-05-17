#ifndef ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_CONVERSION_H
#define ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_CONVERSION_H

#include <string>

#include "includes.h"

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

bool Contains(
    const Edge& e, 
    const edge* list, 
    const uint size
    )
{
    uint count = 0;
    for(uint i =0; i < size; i++)
    {
        if(list[i].s == e.u && list[i].t == e.v) count++;
    }

    return count == 1;
}

// This function is different from freespecialsparse but actually works for these tests.
void freespecialsparsenew(specialsparse *ss) {
    free(ss->edges);
    free(ss);
}

TEST_F(ToSpecialSparseTest, CorrectNumberOfNodesDirected)
{
    CLApp cli(0, {}, "stub");
    Builder b(cli);

    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,2);

    CSRGraph g = b.MakeGraphFromEL(list);

    specialsparse* ss = ToSpecialSparse(g);

    ASSERT_EQ(3, ss->n);

    freespecialsparsenew(ss);
}

TEST_F(ToSpecialSparseTest, CorrectNumberOfNodesUndirected)
{
    UCLApp cli(0, {}, "stub");
    Builder b(cli);

    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,3);

    CSRGraph g = b.MakeGraphFromEL(list);

    specialsparse* ss = ToSpecialSparse(g);

    ASSERT_EQ(4, ss->n);

    freespecialsparsenew(ss);
}

TEST_F(ToSpecialSparseTest, CorrectNumberOfEdgesDirected)
{
    CLApp cli(0, {}, "stub");
    Builder b(cli);

    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,2);

    CSRGraph g = b.MakeGraphFromEL(list);

    specialsparse* ss = ToSpecialSparse(g);

    ASSERT_EQ(3, ss->e);

    freespecialsparsenew(ss);
}

TEST_F(ToSpecialSparseTest, CorrectNumberOfEdgesUndirected)
{
    UCLApp cli(0, {}, "stub");
    cli.ParseArgs();
    Builder b(cli);

    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,3);

    CSRGraph g = b.MakeGraphFromEL(list);

    EXPECT_FALSE( g.directed() );

    specialsparse* ss = ToSpecialSparse(g);

    EXPECT_EQ(3, ss->e);
    EXPECT_EQ(4, ss->n);

    freespecialsparsenew(ss);
}

TEST_F(ToSpecialSparseTest, EdgeArrayIsAllocated)
{
    CLApp cli(0, {}, "stub");
    Builder b(cli);

    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,2);

    CSRGraph g = b.MakeGraphFromEL(list);

    specialsparse* ss = ToSpecialSparse(g);

    // should not throw any exception
    auto edge = ss->edges[2];
    EXPECT_EQ( edge.s, ss->edges[2].s);

    freespecialsparsenew(ss);
}

TEST_F(ToSpecialSparseTest, SimpleUndirectedGraph)
{
    CLApp cli(0, {}, "stub");
    Builder b(cli);

    EdgeList list(10);
    list[0] = Edge(0,1);
    list[1] = Edge(1,0);
    list[2] = Edge(1,2);
    list[3] = Edge(2,1);
    list[4] = Edge(1,3);
    list[5] = Edge(3,1);
    list[6] = Edge(3,4);
    list[7] = Edge(4,3);
    list[8] = Edge(3,5);
    list[9] = Edge(5,3);

    CSRGraph g = b.MakeGraphFromEL(list);

    specialsparse* ss = ToSpecialSparse(g);

    EXPECT_TRUE( Contains(list[0], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[1], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[2], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[3], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[4], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[5], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[6], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[7], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[8], ss->edges, 10) );
    EXPECT_TRUE( Contains(list[9], ss->edges, 10) );

    freespecialsparsenew(ss);
}

TEST_F(ToSpecialSparseTest, SimpleUndirectedGraphCorrectEdges)
{
    UCLApp cli(0, {}, "stub");
    Builder b(cli);

    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    CSRGraph g= b.MakeGraphFromEL(list);

    specialsparse* ss = ToSpecialSparse(g);

    EXPECT_TRUE( Contains(list[0], ss->edges, ss->e));
    EXPECT_TRUE( Contains(list[1], ss->edges, ss->e));

    freespecialsparsenew(ss);
}

#endif