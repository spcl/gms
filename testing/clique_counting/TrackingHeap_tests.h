#ifndef ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_TRACKINGHEAP_H
#define ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_TRACKINGHEAP_H

#include <iostream>

#include "includes.h"
//#include <preprocessing/util/auxiliary.h>

using Key_T = NodeId;
using Value_T = NodeId;

using namespace coreOrdering;

typedef coreOrdering::KeyValuePair<Key_T, Value_T> KeyValue_T;

TEST(TrackingStdHeap, StrictlyOrderedOnCreation)
{
    KeyValue_T list[4];
    list[0] = {0, 3};
    list[1] = {1, 2};
    list[2] = {2, 4};
    list[3] = {3, 0};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 4);

    KeyValue_T sorted[4];
    for(uint i = 0; i < 4; i++)
    {
        sorted[i] = coll.PopHead();
    }

    EXPECT_EQ( KeyValue_T(3,0), sorted[0]);
    EXPECT_EQ( KeyValue_T(1,2), sorted[1]);
    EXPECT_EQ( KeyValue_T(0,3), sorted[2]);
    EXPECT_EQ( KeyValue_T(2,4), sorted[3]);
}

TEST(TrackingStdHeap, WeaklyOrderedOnCreation)
{
    KeyValue_T list[4];
    list[0] = {0,3};
    list[1] = {1,2};
    list[2] = {2,2};
    list[3] = {3,3};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 4);

    KeyValue_T sorted[4];
    for(uint i =0; i< 4; i++)
    {
        sorted[i] = coll.PopHead();
    }

    EXPECT_EQ(2, sorted[0].Value);
    EXPECT_EQ(2, sorted[1].Value);
    EXPECT_EQ(3, sorted[2].Value);
    EXPECT_EQ(3, sorted[3].Value);
}

TEST(TrackingStdHeap, PopHeadShrinksSize)
{
    KeyValue_T list[1];
    list[0] = {0,3};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 1);

    EXPECT_EQ(1, coll.Size());

    coll.PopHead();

    EXPECT_EQ(0, coll.Size());
}

TEST(TrackingStdHeap, DecreaseValueOfKeyDecreasesValue)
{
    KeyValue_T list[1];
    list[0] = {0,3};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 1);

    coll.DecreaseValueOfKey(0);
    EXPECT_EQ(2, coll.GetKeyValue(0).Value);
}

TEST(TrackingStdHeap, DecraseValueOfKeyReorders)
{
    KeyValue_T list[2];
    list[0] = {0,3};
    list[1] = {1,3};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 2);

    coll.DecreaseValueOfKey(1);
    KeyValue_T sorted[2];
    sorted[0] = coll.PopHead();
    sorted[1] = coll.PopHead();

    EXPECT_EQ( KeyValue_T(1,2), sorted[0]);
    EXPECT_EQ( KeyValue_T(0,3), sorted[1]);
}

TEST(TrackingStdHeap, DecreaseVauleOfKeyKeepsWeakOrder)
{
    KeyValue_T list[5];
    list[0] = {0,0};
    list[1] = {1,1};
    list[2] = {2,1};
    list[3] = {3,2};
    list[4] = {4,2};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 5);

    coll.DecreaseValueOfKey(4);

    EXPECT_EQ(0, coll.PopHead().Value);
    EXPECT_EQ(1, coll.PopHead().Value);
    EXPECT_EQ(1, coll.PopHead().Value);
    EXPECT_EQ(1, coll.PopHead().Value);
    EXPECT_EQ(2, coll.PopHead().Value);
}

TEST(TrackingStdHeap, PopHeadInvaidatesKeyLocation)
{
    KeyValue_T list[1];
    list[0] = {0,2};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 1);

    coll.PopHead();

    EXPECT_EQ(-1, coll.GetIndex(0));
}

TEST(TrackingStdHeap, GetIndexIsCorrect)
{
    KeyValue_T list[4];
    list[0] = {0, 3};
    list[1] = {1, 2};
    list[2] = {2, 4};
    list[3] = {3, 0};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 4);

    for(uint i = 0; i < 4; i++)
    {
        uint idx = coll.GetIndex(i);
        EXPECT_EQ( i, list[idx].Key);
    }
}

TEST(TrackingStdHeap, GetKeyValueReturnsKorrektKeyValue)
{
    KeyValue_T list[4];
    list[0] = {0, 3};
    list[1] = {1, 2};
    list[2] = {2, 4};
    list[3] = {3, 0};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 4);

    for(uint i = 0; i < 4; i++)
    {
        EXPECT_EQ(i, coll.GetKeyValue(i).Key);
    }
}

TEST(TrackingStdHeap, GetKeyValueRefReturnsKorrektKeyValue)
{
    KeyValue_T list[4];
    list[0] = {0, 3};
    list[1] = {1, 2};
    list[2] = {2, 4};
    list[3] = {3, 0};

    auto coll = TrackingStdHeap<NodeId, NodeId>(list, 4);

    for(uint i = 0; i < 4; i++)
    {
        EXPECT_EQ(i, coll.GetKeyValueRef(i).Key);
    }
}


#endif