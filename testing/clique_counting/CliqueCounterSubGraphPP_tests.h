#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_CLIQUECOUNTERSUBGRAPHPP_H
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_CLIQUECOUNTERSUBGRAPHPP_H

#include "includes.h"

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

class CliqueCounterSubGraphPPFixture : public ::testing::Test
{
    protected:
    UCLApp ucli_;
    CLApp dcli_;

    public:
    CliqueCounterSubGraphPPFixture()
    : ucli_(UCLApp(0, nullptr, "stub")), dcli_(CLApp(0, nullptr, "stub"))
    {}
    ~CliqueCounterSubGraphPPFixture() {}

    cc::Graph_T DirGraph(EdgeList& list)
    {
        BuilderBase<NodeId> builder(dcli_);
        return builder.MakeGraphFromEL(list);
    }

    cc::Graph_T UndirGraph(EdgeList& list)
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

TEST_F(CliqueCounterSubGraphPPFixture, Counts2CliquesCorrect)
{
    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    cc::Graph_T g = DirGraph(list);
    FixedCLApp cli(2);
    Counter::DegreeK counter(2, g);

    ASSERT_EQ(2, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, CountsMany2CliquesCorrect)
{
    EdgeList list(5);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(0,5);

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(2, g);

    ASSERT_EQ(5, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, CountsNo3Clique)
{
    EdgeList list(2);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(3, g);

    ASSERT_EQ(0, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, CountsNo3Clique2)
{
    EdgeList list(5);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,3);
    list[3] = Edge(0,4);
    list[4] = Edge(0,5);

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(3, g);

    ASSERT_EQ(0, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, Counts3Clique)
{
    EdgeList list(3);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(0,2);

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(3, g);

    ASSERT_EQ(1, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, CountsMany3CliquesCorrect)
{
    EdgeList list(12);
    list[0] = Edge(1,2);
    list[1] = Edge(2,3);
    list[2] = Edge(3,4);
    list[3] = Edge(4,5);
    list[4] = Edge(5,6);
    list[5] = Edge(1,6);

    list[6] = Edge(0,1);
    list[7] = Edge(0,2);
    list[8] = Edge(0,3);
    list[9] = Edge(0,4);
    list[10] = Edge(0,5);
    list[11] = Edge(0,6);

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(3, g);

    ASSERT_EQ(6, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, CountsNo4Clique)
{
    EdgeList list(12);
    list[0] = Edge(1,2);
    list[1] = Edge(2,3);
    list[2] = Edge(3,4);
    list[3] = Edge(4,5);
    list[4] = Edge(5,6);
    list[5] = Edge(1,6);

    list[6] = Edge(0,1);
    list[7] = Edge(0,2);
    list[8] = Edge(0,3);
    list[9] = Edge(0,4);
    list[10] = Edge(0,5);
    list[11] = Edge(0,6);

    cc::Graph_T g = DirGraph(list);
    g.PrintTopology();
    Counter::DegreeK counter(5, g);

    ASSERT_EQ(0, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, Counts4CliquesCorrect)
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

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(4, g);

    ASSERT_EQ(6, counter.count(g));
}

TEST_F(CliqueCounterSubGraphPPFixture, Counts4CliquesCorrect2)
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

    cc::Graph_T g = DirGraph(list);
    Counter::DegreeK counter(4, g);

    ASSERT_EQ(4, counter.count(g));
}


#endif