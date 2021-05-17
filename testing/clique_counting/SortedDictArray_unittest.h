#ifndef ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_SORTEDDICTARRAY_H
#define ABSTRACTIONOPTIMIZING_TESTS_MINEBENCH_CLIQUECOUNTING_SORTEDDICTARRAY_H

#include <iostream>

#include "includes.h"

std::ostream& operator<<(std::ostream& os, const KeyValue& kv)
{
    os << "(" << kv.Key << "/" << kv.Value << ")";
    return os;
}

TEST(SortedDictArray, IsOrderedOnCreation)
{
    KeyValue list[3];
    list[0] = {0, 5};
    list[1] = {1, 3};
    list[2] = {2, 4};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&(list[0]), 3, c);

    KeyValue val0 = {1,3};
    KeyValue val1 = {2,4};
    KeyValue val2 = {0,5};

    ASSERT_TRUE( c.equiv(val0, list[0]) ) << val0 << ", " << list[0] << std::endl;
    ASSERT_TRUE( c.equiv(val1, list[1]) ) << val1 << ", " << list[1] << std::endl;
    ASSERT_TRUE( c.equiv(val2, list[2]) ) << val2 << ", " << list[2] << std::endl;

}

TEST(SortedDictArray, IsWeaklyOrderedOnCreation)
{
    KeyValue list[3];
    list[0] = {0, 5};
    list[1] = {1, 4};
    list[2] = {2, 4};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 3, c);

    KeyValue val0 = {1,4};
    KeyValue val1 = {2,4};
    KeyValue val2 = {0,5};

    ASSERT_TRUE( c.equiv(val0, list[0]) ) << val0 << ", " << list[0] << std::endl;
    ASSERT_TRUE( c.equiv(val1, list[1]) ) << val1 << ", " << list[1] << std::endl;
    ASSERT_TRUE( c.equiv(val2, list[2]) ) << val2 << ", " << list[2] << std::endl;
}

TEST(SortedDictArray, PopMin_popsMin)
{
    KeyValue list[3];
    list[0] = {0, 3};
    list[1] = {1, 4};
    list[2] = {2, 5};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 3, c);
    KeyValue min = sorted.getStart();

    ASSERT_TRUE( min.Key == 0 && min.Value == 3);
}

TEST(SortedDictArray, PopMin_shrinksSize)
{
    KeyValue list[3];
    list[0] = {0, 3};
    list[1] = {1, 4};
    list[2] = {2, 5};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 3, c);
    KeyValue min = sorted.getStart();

    ASSERT_EQ( 2, sorted.Length() );
}

TEST(SortedDictArray, ValueIsDecreased)
{
    KeyValue list[1];
    list[0] = {0,2};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 1, c );

    sorted.decreaseValueOfKey(0);

    ASSERT_EQ( 1, sorted.getValueAtIndex(0));
}

TEST(SortedDictArray, ValueBeforeStartIsNotChanged)
{
    KeyValue list[2];
    list[0] = {0,2};
    list[1] = {1,3};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 2, c);
    sorted.getStart();

    sorted.decreaseValueOfKey(0);

    ASSERT_EQ(2, list[0].Value);
}

TEST(SortedDictArray, StillSortedAfterDecreaseSimpleCase)
{
    KeyValue list[3];
    list[0] = {0, 1};
    list[1] = {1, 3};
    list[2] = {2, 3};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 3, c);

    uint key = sorted.getKeyValueAtIndex(2).Key;
    sorted.decreaseValueOfKey(key);

    KeyValue val0 = {0,1};
    KeyValue val1 = {2,2};
    KeyValue val2 = {1,3};

    EXPECT_TRUE( c.equiv( val0, list[0]) ) << val0 << ", " << list[0] << std::endl;
    EXPECT_TRUE( c.equiv( val1, list[1]) ) << val1 << ", " << list[1] << std::endl;
    EXPECT_TRUE( c.equiv( val2, list[2]) ) << val2 << ", " << list[2] << std::endl;
}

TEST(SortedDictArray, StillSortedAfterDecreaseComplexCase)
{
    KeyValue list[7];
    list[0] = {0, 1};
    list[1] = {1, 2};
    list[2] = {2, 2};
    list[3] = {3, 3};
    list[4] = {4, 3};
    list[5] = {5, 3};
    list[6] = {6, 4};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 7, c);
    uint key = sorted.getKeyValueAtIndex(5).Key;

    KeyValue val0 = {0, 1};
    KeyValue val1 = {1, 2};
    KeyValue val2 = {2, 2};
    KeyValue val3 = {5, 2};
    KeyValue val4 = {3, 3};
    KeyValue val5 = {4, 3};
    KeyValue val6 = {6, 4};

    

    sorted.decreaseValueOfKey(key);

    EXPECT_TRUE( c.equiv(val0, list[0])) << val0 << ", " << list[0] << std::endl;
    EXPECT_TRUE( c.equiv(val1, list[1])) << val1 << ", " << list[1] << std::endl;
    EXPECT_TRUE( c.equiv(val2, list[2])) << val2 << ", " << list[2] << std::endl;
    EXPECT_TRUE( c.equiv(val3, list[3])) << val3 << ", " << list[3] << std::endl;
    EXPECT_TRUE( c.equiv(val4, list[4])) << val4 << ", " << list[4] << std::endl;
    EXPECT_TRUE( c.equiv(val5, list[5])) << val5 << ", " << list[5] << std::endl;
    EXPECT_TRUE( c.equiv(val6, list[6])) << val6 << ", " << list[6] << std::endl;
}

TEST(SortedDictArray, BubbleUPCorrectSimpleCase)
{
    KeyValue list[4];
    list[0] = {0,1};
    list[1] = {4,2};
    list[2] = {3,2};
    list[3] = {2,3};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 4, c);

    sorted.decreaseValueOfKey(2);

   uint x = sorted.getValueOfKey(2);
   uint index = sorted.indexOfKey(2);

   EXPECT_EQ( 3, index);
   EXPECT_EQ( 2, x); 
}

TEST(SortedDictArray, BubbleUpCorrectComplexCase)
{
    KeyValue list[6];
    list[0] = {5, 1};
    list[1] = {4, 2};
    list[2] = {3, 3};
    list[3] = {2, 3};
    list[4] = {1, 4};
    list[5] = {0, 4};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 6, c);

    sorted.decreaseValueOfKey(1);

    uint index = sorted.indexOfKey(1);
    uint value = sorted.getValueOfKey(1);

    KeyValue val0 = {5,1};
    KeyValue val1 = {4,2};
    KeyValue val2 = {3,3};
    KeyValue val3 = {2,3};
    KeyValue val4 = {1,3};
    KeyValue val5 = {0,4};

    EXPECT_TRUE( c.equiv(val0, list[0])) << val0 << ", " << list[0] << std::endl;
    EXPECT_TRUE( c.equiv(val1, list[1])) << val1 << ", " << list[1] << std::endl;
    EXPECT_TRUE( c.equiv(val2, list[2])) << val2 << ", " << list[2] << std::endl;
    EXPECT_TRUE( c.equiv(val3, list[3])) << val3 << ", " << list[3] << std::endl;
    EXPECT_TRUE( c.equiv(val4, list[4])) << val4 << ", " << list[4] << std::endl;
    EXPECT_TRUE( c.equiv(val5, list[5])) << val5 << ", " << list[5] << std::endl;  
}

TEST(SortedDictArray, BubbleUpCorrectComplexCase2)
{
    KeyValue list[6];
    list[0] = {5, 1};
    list[1] = {4, 2};
    list[2] = {3, 3};
    list[3] = {2, 3};
    list[4] = {1, 4};
    list[5] = {0, 4};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 6, c);

    sorted.decreaseValueOfKey(0);

    KeyValue val0 = {5,1};
    KeyValue val1 = {4,2};
    KeyValue val2 = {3,3};
    KeyValue val3 = {2,3};
    KeyValue val4 = {0,3};
    KeyValue val5 = {1,4};

    EXPECT_TRUE( c.equiv(val0, list[0])) << val0 << ", " << list[0] << std::endl;
    EXPECT_TRUE( c.equiv(val1, list[1])) << val1 << ", " << list[1] << std::endl;
    EXPECT_TRUE( c.equiv(val2, list[2])) << val2 << ", " << list[2] << std::endl;
    EXPECT_TRUE( c.equiv(val3, list[3])) << val3 << ", " << list[3] << std::endl;
    EXPECT_TRUE( c.equiv(val4, list[4])) << val4 << ", " << list[4] << std::endl;
    EXPECT_TRUE( c.equiv(val5, list[5])) << val5 << ", " << list[5] << std::endl;  
}

TEST(SortedDictArray, FetchCorrectIndexOfShrinkedArray)
{
    KeyValue list[4];
    list[0] = {0,1};
    list[1] = {1,2};
    list[2] = {2,3};
    list[3] = {3,3};

    auto c = NodeComparerMax();
    auto sorted = SortedDictArray(&list[0], 4, c);

    sorted.getStart();

    KeyValue val0 = {1,2};
    KeyValue val1 = {2,3};
    KeyValue val2 = {3,3};

    EXPECT_TRUE( c.equiv(val0, sorted.getKeyValueAtIndex(1))) 
        << val0 << "/" << sorted.getKeyValueAtIndex(1) << std::endl;
    EXPECT_TRUE( c.equiv(val1, sorted.getKeyValueAtIndex(2))) 
        << val1 << "/" << sorted.getKeyValueAtIndex(2) << std::endl;
    EXPECT_TRUE( c.equiv(val2, sorted.getKeyValueAtIndex(3))) 
        << val2 << "/" << sorted.getKeyValueAtIndex(3) << std::endl;
}

#endif