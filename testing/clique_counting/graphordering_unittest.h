#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_GRAPHORDERING_H
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_GRAPHORDERING_H


#include "includes.h"

using namespace auxiliary;

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

class GraphOrderingFixture : public ::testing::Test
{
    Builder* ub_;
    Builder* db_;
    protected:

    Builder& UndirB() { return *ub_;}

    Builder& DirB() { return *db_;}
    

    public:
    GraphOrderingFixture() :
    ub_(nullptr), db_(nullptr) {}
    ~GraphOrderingFixture() {}

    virtual void SetUp() override 
    {
        UCLApp ucl(0, nullptr, "stub");
        Builder ub(ucl);
        ub_ = &ub;

        CLApp dcl(0, nullptr, "stub");
        Builder db(dcl);
        db_ = &db;
    }

    virtual void TearDown() override
    {
        ub_ = nullptr;
        db_ = nullptr;
    }
};

TEST_F(GraphOrderingFixture, ComputesDegeneracyOrderSmallUndirectedGraph)
{
    EdgeList list(7);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(2,3);
    list[3] = EdgePair(2,5);
    list[4] = EdgePair(3,4);
    list[5] = EdgePair(3,5);
    list[6] = EdgePair(4,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    EXPECT_EQ( 5, ranking[0]);
    EXPECT_EQ( 4, ranking[1]);
    EXPECT_EQ( 2, ranking[2]);
    EXPECT_EQ( 1, ranking[3]);
    EXPECT_EQ( 3, ranking[4]);
    EXPECT_EQ( 0, ranking[5]);
}

TEST_F(GraphOrderingFixture, ComputesDegeneracyOrderMoreComplexUndirectedGraph)
{
    EdgeList list(10);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(1,3);
    list[3] = EdgePair(2,3);
    list[4] = EdgePair(2,4);
    list[5] = EdgePair(3,4);
    list[6] = EdgePair(3,5);
    list[7] = EdgePair(4,5);
    list[8] = EdgePair(5,6);
    list[9] = EdgePair(6,7);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    EXPECT_EQ(7, ranking[0]);
    EXPECT_EQ(4, ranking[1]);
    EXPECT_EQ(2, ranking[2]);
    EXPECT_EQ(0, ranking[3]);
    EXPECT_EQ(1, ranking[4]);
    EXPECT_EQ(3, ranking[5]);
    EXPECT_EQ(5, ranking[6]);
    EXPECT_EQ(6, ranking[7]);
}

#endif