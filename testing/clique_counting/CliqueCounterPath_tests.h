#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_CLIQUECOUNTERPATH_H
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_CLIQUECOUNTERPATH_H

#include "includes.h"

using namespace auxiliary;
using namespace coreOrdering;

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

typedef TrackingStdHeap<NodeId, uint, NodeComparerMin> Sorting_T;
typedef DegeneracyOrderer<NodeId, NodeComparerMin, Sorting_T> Order_T;


class CliqueCounterPathsFixture : public ::testing::Test
{
    protected:
    UCLApp ucli_;
    CLApp dcli_;

    public:
    CliqueCounterPathsFixture()
    : ucli_(UCLApp(0, nullptr, "stub")), dcli_(CLApp(0, nullptr, "stub"))
    {}
    ~CliqueCounterPathsFixture() {}

    CSRGraph DirGraph(EdgeList& list)
    {
        BuilderBase<NodeId> builder(dcli_);
        return builder.MakeGraphFromEL(list);
    }

    CSRGraph UndirGraph(EdgeList& list)
    {
        BuilderBase<NodeId> builder(ucli_);
        return builder.MakeGraphFromEL(list);
    }

    virtual void SetUp() override 
    {
    }

    virtual void TearDown() override
    {
    }

};

TEST_F(CliqueCounterPathsFixture, Counts2CliquesCorrect)
{
    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(2);

    ASSERT_EQ(2, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, CountsMany2CliquesCorrect)
{
    EdgeList list(5);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(0,5);

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(2);

    ASSERT_EQ(5, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, CountsNo3Clique)
{
    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(3);

    ASSERT_EQ(0, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, CountsNo3Clique2)
{
    EdgeList list(5);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(0,5);

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(3);

    ASSERT_EQ(0, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, Counts3Clique)
{
    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,0);

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(3);

    ASSERT_EQ(1, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, CountsMany3CliquesCorrect)
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

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(3);

    uint count = counter.count(gdir);

    ASSERT_EQ(6, count);
}

TEST_F(CliqueCounterPathsFixture, CountsNo4Clique)
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

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(4);

    ASSERT_EQ(0, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, Counts4CliquesCorrect)
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

    CSRGraph g = UndirGraph(list);

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(4);

    ASSERT_EQ(6, counter.count(gdir));
}

TEST_F(CliqueCounterPathsFixture, Counts4CliquesCorrect2)
{
    EdgeList list(0);
    list.push_back( Edge(0,1));
    list.push_back( Edge(0,2));
    list.push_back( Edge(0,3));
    list.push_back( Edge(1,2));
    list.push_back( Edge(1,3));
    list.push_back( Edge(2,3));
    list.push_back( Edge(2,8));
    list.push_back( Edge(3,12));
    list.push_back( Edge(4,5));
    list.push_back( Edge(4,6));
    list.push_back( Edge(4,7));
    list.push_back( Edge(4,9));
    list.push_back( Edge(5,6));
    list.push_back( Edge(5,7));
    list.push_back( Edge(6,7));
    list.push_back( Edge(6,12));
    list.push_back( Edge(7,13));
    list.push_back( Edge(8,9));
    list.push_back( Edge(8,10));
    list.push_back( Edge(8,11));
    list.push_back( Edge(9,10));
    list.push_back( Edge(9,11));
    list.push_back( Edge(10,11));
    list.push_back( Edge(11,14));
    list.push_back( Edge(12,13));
    list.push_back( Edge(12,14));
    list.push_back( Edge(12,15));
    list.push_back( Edge(13,14));
    list.push_back( Edge(13,15));
    list.push_back( Edge(14,15));

    CSRGraph g = UndirGraph(list);
    EXPECT_FALSE( g.directed());

    pvector<uint> ranking(g.num_nodes());
    Order_T ranker(g, ranking);
    ranker.ComputeRanking();

    CSRGraph gdir = GraphDirecter().MakeDirected(g, ranking);

    CliqueCounterPaths<NodeId> counter(4);

    ASSERT_EQ(4, counter.count(gdir));
}


#endif