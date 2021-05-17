#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_KCLIST_H
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_KCLIST_H

#include "includes.h"

using namespace auxiliary;

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

class CliqueCounterFixture : public ::testing::Test
{
    protected:
    Builder* ub_; //builder for undirected graphs
    Builder* db_; //builder for directed graphs

    public:
    CliqueCounterFixture() {}
    ~CliqueCounterFixture() {}

    Builder& DirB()
    {
        return *db_;
    }

    Builder& UndirB()
    {
        return *ub_;
    }

    virtual void SetUp() override 
    {
        UCLApp ucli(0, nullptr, "stub");
        Builder ub(ucli);
        ub_ = &ub;

        CLApp dcli(0, nullptr, "stub");
        Builder db(dcli);
        db_ = &db;
    }

    virtual void TearDown() override
    {
        ub_ = nullptr;
        db_ = nullptr;
    }

};

TEST_F(CliqueCounterFixture, CountsUndirected2CliquesCorrect)
{
    EdgeList list(2);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 2> counter(g, ranking);

    ASSERT_EQ(2, counter.count());
}

TEST_F(CliqueCounterFixture, CountsManyUndirected2CliquesCorrect)
{
    EdgeList list(5);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(0,3);
    list[3] = EdgePair(0,4);
    list[4] = EdgePair(0,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 2> counter(g, ranking);

    ASSERT_EQ(5, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNoUndirected3Clique)
{
    EdgeList list(2);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ(0, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNo3CliqueInLargerUndirectedGraph)
{
    EdgeList list(5);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(0,3);
    list[3] = EdgePair(0,4);
    list[4] = EdgePair(0,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ(0, counter.count());
}



TEST_F(CliqueCounterFixture, CountUndirected3CliqueCorrect)
{
    EdgeList list(3);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(2,0);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ(1, counter.count());
}

TEST_F(CliqueCounterFixture, CountsManyUndirected3CliquesCorrect)
{
    EdgeList list(12);
    list[0] = EdgePair(1,2);
    list[1] = EdgePair(2,3);
    list[2] = EdgePair(3,4);
    list[3] = EdgePair(4,5);
    list[4] = EdgePair(5,6);
    list[5] = EdgePair(6,1);

    list[6] = EdgePair(0,1);
    list[7] = EdgePair(0,2);
    list[8] = EdgePair(0,3);
    list[9] = EdgePair(0,4);
    list[10] = EdgePair(0,5);
    list[11] = EdgePair(0,6);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ( 6, counter.count());
}

TEST_F(CliqueCounterFixture, CountsDirected2CliqueCorrect)
{
    EdgeList list(1);
    list[0] = EdgePair(0,1);

    CSRGraph g = DirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 2> counter(g, ranking);

    ASSERT_EQ(1, counter.count());
}

TEST_F(CliqueCounterFixture, CountsManyDirected2CliquesCorrect)
{
    EdgeList list(5);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(0,3);
    list[3] = EdgePair(0,4);
    list[4] = EdgePair(0,5);

    CSRGraph g = DirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 2> counter(g, ranking);

    ASSERT_EQ(5, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNoDirected3Clique)
{
    EdgeList list(2);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);

    CSRGraph g = DirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ(0, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNo3CliqueINLargerDirectedGraph)
{
    EdgeList list(5);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(0,3);
    list[3] = EdgePair(0,4);
    list[4] = EdgePair(0,5);

    CSRGraph g = DirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ(0, counter.count());
}

TEST_F(CliqueCounterFixture, CountDirected3CliquesCorrect)
{
    EdgeList list(3);
    list[0] = EdgePair(0,1);
    list[1] = EdgePair(1,2);
    list[2] = EdgePair(2,0);

    CSRGraph g = DirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ(1, counter.count());
}

TEST_F(CliqueCounterFixture, CountsManydirected3CliquesCorrect)
{
    EdgeList list(12);
    list[0] = EdgePair(1,2);
    list[1] = EdgePair(2,3);
    list[2] = EdgePair(3,4);
    list[3] = EdgePair(4,5);
    list[4] = EdgePair(5,6);
    list[5] = EdgePair(6,1);

    list[6] = EdgePair(0,1);
    list[7] = EdgePair(0,2);
    list[8] = EdgePair(0,3);
    list[9] = EdgePair(0,4);
    list[10] = EdgePair(0,5);
    list[11] = EdgePair(0,6);

    CSRGraph g = DirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    findDegeneracyRanking(g, ranking);

    CliqueCounter<NodeId, 3> counter(g, ranking);

    ASSERT_EQ( 6, counter.count());
}

#endif 