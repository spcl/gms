#ifndef ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_COMPARER_H
#define ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_COMPARER_H

#include <algorithm>

#include "includes.h"
//#include <gms/preprocessing/util/auxiliary.h>

using namespace coreOrdering;

typedef KeyValuePair<NodeId, NodeId> KeyValue;

TEST(NodeComparerMax, FalseIfLarger)
{
    KeyValue a = {0, 2};
    KeyValue b = {1, 1};

    auto c = NodeComparerMax();

    ASSERT_FALSE( c.cmp(a,b) );
}

TEST(NodeComparerMax, FalseIfEqual)
{
    KeyValue a = {0, 1};
    KeyValue b = {1, 1};

    auto c = NodeComparerMax();

    ASSERT_FALSE( c.cmp(a,b) );
}

TEST(NodeComparerMax, TrueIfSmaller)
{
    KeyValue a = {0, 1};
    KeyValue b = {1, 2};

    auto c = NodeComparerMax();

    ASSERT_TRUE( c.cmp(a,b) );
}

TEST(NodeComparerMin, TrueIfLarger)
{
    KeyValue a = {0, 2};
    KeyValue b = {1, 1};

    auto c = NodeComparerMin();

    ASSERT_TRUE( c.cmp(a,b) );
}

TEST(NodeComparerMin, FalseIfEqual)
{
    KeyValue a = {0, 1};
    KeyValue b = {1, 1};

    auto c = NodeComparerMin();

    ASSERT_FALSE( c.cmp(a,b) );
}

TEST(NodeComparerMin, FalseIfSmaller)
{
    KeyValue a = {0, 1};
    KeyValue b = {1, 2};

    auto c = NodeComparerMin();

    ASSERT_FALSE( c.cmp(a,b) );
}

// TEST(RankOrderComparer, FalseIfLarger)
// {
//     pvector<uint> order(2);
//     order[0] = 0;
//     order[1] = 1;

//     auto c = RankOrderComparer(order);

//     ASSERT_FALSE( c.cmp(1,0) );
// }

// TEST(RankOrderComparer, FalseIfEqual)
// {
//     pvector<uint> order(2);
//     order[0] = 0;
//     order[1] = 1;

//     auto c = RankOrderComparer(order);

//     ASSERT_FALSE( c.cmp(0,0) );
// }

// TEST(RankOrderComparer, TrueIfSmaller)
// {
//     pvector<uint> order(2);
//     order[0] = 0;
//     order[1] = 1;

//     auto c = RankOrderComparer(order);

//     ASSERT_TRUE( c.cmp(0,1) );
// }

TEST(Sort, SortsNonDecreasing)
{
    KeyValue kv[3];
    kv[0] = {0, 2};
    kv[1] = {1, 2};
    kv[2] = {2, 3};

    auto c = NodeComparerMax();

    std::sort(kv, kv+3, c);

    EXPECT_EQ( KeyValue(0,2), kv[0]);
    EXPECT_EQ( KeyValue(1,2), kv[1]);
    EXPECT_EQ( KeyValue(2,3), kv[2]);
}

TEST(Sort, SortsNonDecreasing2)
{
    KeyValue kv[3];
    kv[0] = {0,2};
    kv[1] = {1,1};
    kv[2] = {2,1};

    auto c = NodeComparerMax();

    std::sort(kv, kv+3,c);

    EXPECT_EQ( KeyValue(1,1), kv[0]);
    EXPECT_EQ( KeyValue(2,1), kv[1]);
    EXPECT_EQ( KeyValue(0,2), kv[2]);
}

#endif