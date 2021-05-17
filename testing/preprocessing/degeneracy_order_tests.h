#ifndef ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_DECENERACYORDERER_H
#define ABSTRACTIONOPTIMIZING_TEST_MINEBENCH_CLIQUECOUNTING_DECENERACYORDERER_H

#include "../test_helper.h"
#include "../clique_counting/UCLApp.h"

#include <gms/algorithms/preprocessing/preprocessing.h>
// TODO (this all should probably be refactored, also in the main GMS code there are also a couple instances of this)

typedef EdgePair<NodeId, NodeId> Edge;
typedef pvector<Edge> EdgeList;

class DegeneracyOrdererFixture : public ::testing::Test
{
    UCLApp ucl = UCLApp(0, nullptr, "undir");
    CLApp dcl = CLApp(0, nullptr, "dir");

protected:
    Builder UndirB() {return Builder(ucl);}
    Builder DirB() {return Builder(dcl);}

public:
    DegeneracyOrdererFixture() {}
    ~DegeneracyOrdererFixture() {}

    virtual void SetUp() override 
    {
    }

    virtual void TearDown() override
    {
    }
};



TEST_F(DegeneracyOrdererFixture, ComputesRankingSmallDirectedGraph)
{
    EdgeList list(7);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,3);
    list[3] = Edge(2,5);
    list[4] = Edge(3,4);
    list[5] = Edge(3,5);
    list[6] = Edge(4,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);
    std::vector<NodeId> ranking;
    PpSequential::getDegeneracyOrderingDanischHeap(g, ranking);

    EXPECT_EQ( 5, ranking[0]);
    EXPECT_EQ( 4, ranking[1]);
    EXPECT_EQ( 2, ranking[2]);
    EXPECT_EQ( 1, ranking[3]);
    EXPECT_EQ( 3, ranking[4]);
    EXPECT_EQ( 0, ranking[5]);
}

TEST_F(DegeneracyOrdererFixture, ComputesRakingMoreCommplexDirectedGraph)
{
    EdgeList list(9);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(1,3);
    list[3] = Edge(2,3);
    list[4] = Edge(2,4);
    list[5] = Edge(3,4);
    list[6] = Edge(3,5);
    list[7] = Edge(4,5);
    list[8] = Edge(5,6);
    //list[9] = Edge(6,7);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    std::vector<NodeId> ranking;
    PpSequential::getDegeneracyOrderingDanischHeap(g, ranking);

    EXPECT_TRUE( ranking[0] > ranking[1]);
    EXPECT_TRUE( ranking[6] > ranking[1]);
    EXPECT_TRUE( ranking[0] > ranking[4]);
    EXPECT_TRUE( ranking[6] > ranking[4]);
    EXPECT_TRUE( ranking[0] > ranking[5]);
    EXPECT_TRUE( ranking[6] > ranking[5]);

    EXPECT_TRUE( ranking[1] > ranking[2]);
    EXPECT_TRUE( ranking[4] > ranking[2]);
    EXPECT_TRUE( ranking[5] > ranking[2]);
    EXPECT_TRUE( ranking[1] > ranking[3]);
    EXPECT_TRUE( ranking[4] > ranking[3]);
    EXPECT_TRUE( ranking[5] > ranking[3]);
}

TEST_F(DegeneracyOrdererFixture, ComputesRankingTest3)
{
    EdgeList list(15);
    list[0] = Edge(0,1);
    list[1] = Edge(0,2);
    list[2] = Edge(1,3);
    list[3] = Edge(2,3);
    list[4] = Edge(3,4);
    list[5] = Edge(3,5);
    list[6] = Edge(4,5);
    list[7] = Edge(4,6);
    list[8] = Edge(4,8);
    list[9] = Edge(5,6);
    list[10]= Edge(6,7);
    list[11]= Edge(6,8);
    list[12]= Edge(7,8);
    list[13]= Edge(7,9);
    list[14]= Edge(8,9);

    CSRGraph g = UndirB().MakeGraphFromEL(list);

    std::vector<NodeId> ranking;
    PpSequential::getDegeneracyOrderingDanischHeap(g, ranking);

    EXPECT_TRUE( ranking[0] > ranking[3]);
    EXPECT_TRUE( ranking[1] > ranking[3]);
    EXPECT_TRUE( ranking[2] > ranking[3]);
    EXPECT_TRUE( ranking[9] > ranking[3]);
    EXPECT_TRUE( ranking[0] > ranking[7]);
    EXPECT_TRUE( ranking[1] > ranking[7]);
    EXPECT_TRUE( ranking[2] > ranking[7]);
    EXPECT_TRUE( ranking[9] > ranking[7]);

    EXPECT_TRUE( ranking[3] > ranking[5]);
    EXPECT_TRUE( ranking[7] > ranking[5]);
    EXPECT_TRUE( ranking[3] > ranking[8]);
    EXPECT_TRUE( ranking[7] > ranking[8]);

    EXPECT_TRUE( ranking[5] > ranking[4]);
    EXPECT_TRUE( ranking[8] > ranking[4]);
    EXPECT_TRUE( ranking[5] > ranking[6]);
    EXPECT_TRUE( ranking[8] > ranking[6]);
}

TEST_F(DegeneracyOrdererFixture, TestTypeDefCoreOrderingHeap)
{
    EdgeList list(7);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,3);
    list[3] = Edge(2,5);
    list[4] = Edge(3,4);
    list[5] = Edge(3,5);
    list[6] = Edge(4,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);
    std::vector<NodeId> ranking;
    PpSequential::getDegeneracyOrderingDanischHeap(g, ranking);

    EXPECT_EQ( 5, ranking[0]);
    EXPECT_EQ( 4, ranking[1]);
    EXPECT_EQ( 2, ranking[2]);
    EXPECT_EQ( 1, ranking[3]);
    EXPECT_EQ( 3, ranking[4]);
    EXPECT_EQ( 0, ranking[5]);
}

TEST_F(DegeneracyOrdererFixture, TestTypeDefCoreOrderingBubbling)
{
    EdgeList list(7);
    list[0] = Edge(0,1);
    list[1] = Edge(1,2);
    list[2] = Edge(2,3);
    list[3] = Edge(2,5);
    list[4] = Edge(3,4);
    list[5] = Edge(3,5);
    list[6] = Edge(4,5);

    CSRGraph g = UndirB().MakeGraphFromEL(list);
    std::vector<NodeId> ranking;
    PpSequential::getDegeneracyOrderingDanischHeap(g, ranking);

    EXPECT_EQ( 5, ranking[0]);
    EXPECT_EQ( 4, ranking[1]);
    EXPECT_EQ( 2, ranking[2]);
    EXPECT_EQ( 1, ranking[3]);
    EXPECT_EQ( 3, ranking[4]);
    EXPECT_EQ( 0, ranking[5]);
}

#endif