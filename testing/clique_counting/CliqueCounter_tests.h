#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_CLIQUECOUNTER_H
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_CLIQUECOUNTER_H

#include "includes.h"

using namespace auxiliary;
using namespace coreOrdering;

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

typedef TrackingStdHeap<NodeId, uint, NodeComparerMin> Sorting_T;
typedef DegeneracyOrderer<NodeId, NodeComparerMin, Sorting_T> Order_T;


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

TEST_F(CliqueCounterFixture, Counts2CliquesCorrect)
{
    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(2, gdir, ranking);

    ASSERT_EQ(2, counter.count());
}

TEST_F(CliqueCounterFixture, CountsMany2CliquesCorrect)
{
    EdgeList list(5);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(0,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(2, gdir, ranking);

    ASSERT_EQ(5, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNo3Clique)
{
    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(3, gdir, ranking);

    ASSERT_EQ(0, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNo3Clique2)
{
    EdgeList list(5);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(0,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(3, gdir, ranking);

    ASSERT_EQ(0, counter.count());
}

TEST_F(CliqueCounterFixture, Counts3Clique)
{
    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,0);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(3, gdir, ranking);

    ASSERT_EQ(1, counter.count());
}

TEST_F(CliqueCounterFixture, CountsMany3CliquesCorrect)
{
    EdgeList list(12);
    list[0] = Edge(1,2);
    list[1] = Edge(2,3);
    list[2] = Edge(3,4);
    list[3] = Edge(4,5);
    list[4] = Edge(5,6);
    list[5] = Edge(6,1);

    list[6] = Edge(0,1);
    list[7] = Edge(0,2);
    list[8] = Edge(0,3);
    list[9] = Edge(0,4);
    list[10] = Edge(0,5);
    list[11] = Edge(0,6);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(3, gdir, ranking);

    ASSERT_EQ(6, counter.count());
}

TEST_F(CliqueCounterFixture, CountsNo4Clique)
{
    EdgeList list(12);
    list[0] = Edge(1,2);
    list[1] = Edge(2,3);
    list[2] = Edge(3,4);
    list[3] = Edge(4,5);
    list[4] = Edge(5,6);
    list[5] = Edge(6,1);

    list[6] = Edge(0,1);
    list[7] = Edge(0,2);
    list[8] = Edge(0,3);
    list[9] = Edge(0,4);
    list[10] = Edge(0,5);
    list[11] = Edge(0,6);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(4, gdir, ranking);

    ASSERT_EQ(0, counter.count());
}

TEST_F(CliqueCounterFixture, Counts4CliquesCorrect)
{
    EdgeList list(19);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(1,2);
    list[5] = Edge(1,3);
    list[6] = Edge(1,4);
    list[7] = Edge(1,5);
    list[8] = Edge(1,6);
    list[9] = Edge(2,3);
    list[10]= Edge(2,4);
    list[11]= Edge(2,5);
    list[12]= Edge(2,6);
    list[13]= Edge(3,4);
    list[14]= Edge(3,7);
    list[15]= Edge(4,8);
    list[16]= Edge(5,6);
    list[17]= Edge(6,7);
    list[18]= Edge(7,8);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounter<NodeId> counter(4, gdir, ranking);

    ASSERT_EQ(6, counter.count());
}


#endif