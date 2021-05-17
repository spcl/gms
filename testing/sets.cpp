#include <gms/representations/sets/sorted_set.h>
#include <gms/representations/sets/roaring_set.h>
#include <gms/representations/sets/robin_hood_set.h>
#include "test_helper.h"

// More information on parameterized tests:
// https://github.com/google/googletest/blob/master/googletest/samples/sample6_unittest.cc

template <class TSet>
class SetsTest : public testing::Test
{};

using testing::UnorderedElementsAre;

using Implementations =
    testing::Types<
        RoaringSet,
        // NOTE: This isn't implemented at this time.
        //Roaring64Set,
        SortedSetBase<std::int32_t>,
        SortedSetBase<std::int64_t>,
        RobinHoodSetBase<std::int32_t>,
        RobinHoodSetBase<std::int64_t>
    >;

TYPED_TEST_SUITE(SetsTest, Implementations);

#define Set TypeParam

TYPED_TEST(SetsTest, Equality)
{
    ASSERT_EQ(Set(), Set());
    ASSERT_EQ(Set({2}), Set({2}));
    ASSERT_NE(Set(), Set({2}));
    ASSERT_NE(Set({2}), Set());

    ASSERT_NE(Set({1}), Set({2}));

    ASSERT_EQ(Set({2, 4, 8}), Set({4, 2, 8}));
    ASSERT_NE(Set({4, 8}), Set({4, 2, 8}));
    ASSERT_NE(Set({2, 4, 8}), Set({2, 8}));

    Set a{2, 3, 4};
    Set b{4, 3, 2};
    Set c;
    Set d;

    ASSERT_EQ(a, b);
    ASSERT_EQ(c, d);
    ASSERT_NE(a, c);
    ASSERT_NE(d, a);
}


TYPED_TEST(SetsTest, Cardinality)
{
    Set a;
    Set b{1, 5, 6};
    ASSERT_EQ(a.cardinality(), 0);
    ASSERT_EQ(b.cardinality(), 3);
}

TYPED_TEST(SetsTest, ConstructVector_Empty)
{
    std::vector<typename Set::SetElement> input{};
    Set set(input);
    ASSERT_EQ(set.cardinality(), 0);
    ASSERT_EQ(set, Set());
}

TYPED_TEST(SetsTest, ConstructVector_NonEmpty)
{
    std::vector<typename Set::SetElement> input = {1, 5, 2, 7, 9, 0, 3};
    Set set(input);
    ASSERT_EQ(set.cardinality(), 7);
    ASSERT_EQ(set, Set({1 ,5, 2, 7, 9, 0, 3}));
}


TYPED_TEST(SetsTest, ConstructPointer_Empty)
{
    std::vector<typename Set::SetElement> input{};
    Set set(input.data(), 0);
    ASSERT_EQ(set.cardinality(), 0);
    ASSERT_EQ(set, Set());
}

TYPED_TEST(SetsTest, ConstructPointer_NonEmpty)
{
    std::vector<typename Set::SetElement> input = {1, 5, 2, 7, 9, 0, 3};
    Set set(input.data(), input.size());
    ASSERT_EQ(set.cardinality(), 7);
    ASSERT_EQ(set, Set({1 ,5, 2, 7, 9, 0, 3}));
}


TYPED_TEST(SetsTest, ConstructSingleton_Various)
{
    Set set1(5);
    ASSERT_EQ(set1, Set({5}));
    ASSERT_EQ(set1.cardinality(), 1);
    Set set2(0);
    ASSERT_EQ(set2, Set({0}));
    ASSERT_EQ(set2.cardinality(), 1);
}


template <class S>
static void test_intersect(const S &a, const S &b, const S &expected)
{
    ASSERT_EQ(a.intersect(b), expected);
    ASSERT_EQ(b.intersect(a), expected);
    ASSERT_EQ(a.intersect_count(b), expected.cardinality());
    ASSERT_EQ(b.intersect_count(a), expected.cardinality());
}

TYPED_TEST(SetsTest, Intersect_EmptyEmpty)
{
    test_intersect(Set(), Set(), Set{});
}

TYPED_TEST(SetsTest, Intersect_EmptyNonempty)
{
    test_intersect(Set({ 1, 2, 3 }), Set(), Set{});
}

TYPED_TEST(SetsTest, Intersect_NoOverlap)
{
    test_intersect(Set({ 1, 2, 3 }), Set({ 4, 5, 6 }), Set({}));
}

TYPED_TEST(SetsTest, Intersect_PartialOverlap)
{
    test_intersect(Set({ 1, 2, 3, 4, 5 }), Set({ 3, 4, 5, 6, 7 }), Set{ 3, 4, 5 });
    test_intersect(Set({ 1, 2, 3, 4, 5, 6, 7 }), Set({ 2, 4, 6, 8 }), Set{ 2, 4, 6 });
}

TYPED_TEST(SetsTest, Intersect_TotalOverlap)
{
    test_intersect(Set({ 1, 2, 3, 4, 5 }), Set({ 1, 2, 3, 4, 5 }), Set{ 1, 2, 3, 4, 5 });
}


template <class S>
static void test_intersect_inplace(const S &a, const S &b, const S &expected)
{
    int64_t size_a = a.cardinality();
    int64_t size_b = b.cardinality();
    S a0 = a.clone();
    a0.intersect_inplace(b);
    ASSERT_EQ(a0, expected);
    S b1 = b.clone();
    b1.intersect_inplace(a);
    ASSERT_EQ(b1, expected);
    // Check that the original sets weren't modified.
    ASSERT_EQ(a.cardinality(), size_a);
    ASSERT_EQ(b.cardinality(), size_b);
}

TYPED_TEST(SetsTest, IntersectInplace_EmptyEmpty)
{
    test_intersect_inplace(Set(), Set(), Set());
}

TYPED_TEST(SetsTest, IntersectInplace_EmptyNonempty)
{
    test_intersect(Set({ 1, 2, 3 }), Set(), Set{});
}

TYPED_TEST(SetsTest, IntersectInplace_NoOverlap)
{
    test_intersect_inplace(Set({ 1, 2, 3 }), Set({ 4, 5, 6 }), Set({}));
}

TYPED_TEST(SetsTest, IntersectInplace_PartialOverlap)
{
    test_intersect_inplace(Set({ 1, 2, 3, 4, 5 }), Set({ 3, 4, 5, 6, 7 }), Set{ 3, 4, 5 });
    test_intersect_inplace(Set({ 1, 2, 3, 4, 5, 6, 7 }), Set({ 2, 4, 6, 8 }), Set{ 2, 4, 6 });
}

TYPED_TEST(SetsTest, IntersectInplace_TotalOverlap)
{
    test_intersect_inplace(Set({ 1, 2, 3, 4, 5 }), Set({ 1, 2, 3, 4, 5 }), Set{ 1, 2, 3, 4, 5 });
}


template <class S>
static void test_union(const S &a, const S &b, const S &expected)
{
    ASSERT_EQ(a.union_with(b), expected);
    ASSERT_EQ(b.union_with(a), expected);
}

TYPED_TEST(SetsTest, Union_EmptyEmpty)
{
    test_union(Set(), Set(), Set());
}

TYPED_TEST(SetsTest, Union_EmptyNonempty)
{
    test_union(Set{ 1, 2, 3 }, Set{}, Set{ 1, 2, 3 });
}

TYPED_TEST(SetsTest, Union_NoOverlap)
{
    test_union(Set({ 1, 2, 3 }), Set({ 4, 5, 6 }), Set{ 1, 2, 3, 4, 5, 6 });
}

TYPED_TEST(SetsTest, Union_PartialOverlap)
{
    test_union(Set({ 1, 2, 3, 4, 5 }), Set({ 3, 4, 5, 6, 8 }), Set{ 1, 2, 3, 4, 5, 6, 8 });
}

TYPED_TEST(SetsTest, Union_TotalOverlap)
{
    test_union(Set({ 1, 2, 3, 4, 5 }), Set({ 1, 2, 3, 4, 5 }), Set{ 1, 2, 3, 4, 5 });
}

TYPED_TEST(SetsTest, Union_WithSingleton)
{
    Set set;
    Set res = set.union_with(2)
        .union_with(5)
        .union_with(4)
        .union_with(8)
        .union_with(0)
        .union_with(25);
    ASSERT_EQ(res, Set({2, 5, 4, 8, 0, 25}));
}

template <class S>
static void test_union_inplace(const S &a, const S &b, const S &expected)
{
    S a_copy = a.clone();
    S b_copy = b.clone();
    a_copy.union_inplace(b);
    ASSERT_EQ(a_copy, expected);
    b_copy.union_inplace(a);
    ASSERT_EQ(b_copy, expected);
}

TYPED_TEST(SetsTest, UnionInplace_EmptyEmpty)
{
    test_union_inplace(Set(), Set(), Set());
}

TYPED_TEST(SetsTest, UnionInplace_Singleton)
{
    Set set;
    set.union_inplace(2);
    ASSERT_EQ(set, Set({2}));
    set.union_inplace(5);
    set.union_inplace(4);
    set.union_inplace(8);
    set.union_inplace(0);
    set.union_inplace(25);
    ASSERT_EQ(set, Set({2, 5, 4, 8, 0, 25}));
    set.union_inplace(25);
    ASSERT_EQ(set, Set({2, 5, 4, 8, 0, 25}));
}

template <class S>
static void test_union_count(const S &a, const S &b, uint64_t expected)
{
    uint64_t count = a.union_count(b);
    ASSERT_EQ(count, expected);
}

TYPED_TEST(SetsTest, UnionCount_EmptyEmpty)
{
    test_union_count(Set(), Set(), 0);
}

TYPED_TEST(SetsTest, UnionCount_EmptyNonempty)
{
    test_union_count(Set{ 1, 2, 3 }, Set{}, 3);
}

TYPED_TEST(SetsTest, UnionCount_NoOverlap)
{
    test_union_count(Set({ 1, 2, 3 }), Set({ 4, 5, 6 }), 6);
}

TYPED_TEST(SetsTest, UnionCount_PartialOverlap)
{
    test_union_count(Set({ 1, 2, 3, 4, 5 }), Set({ 3, 4, 5, 6, 8 }), 7);
}

TYPED_TEST(SetsTest, UnionCount_TotalOverlap)
{
    test_union_count(Set({ 1, 2, 3, 4, 5 }), Set({ 1, 2, 3, 4, 5 }), 5);
}

template <class S>
static void test_difference(const S &a, const S &b,
                            const S &left_diff, const S &right_diff)
{
    ASSERT_EQ(a.difference(b), left_diff);
    ASSERT_EQ(b.difference(a), right_diff);
}

TYPED_TEST(SetsTest, Difference_EmptyEmpty)
{
    test_difference(Set(), Set(), Set{}, Set{});
}

TYPED_TEST(SetsTest, Difference_EmptyNonempty)
{
    test_difference(Set({ 1, 2, 3 }), Set(), Set{ 1, 2, 3 }, Set{});
}

TYPED_TEST(SetsTest, Difference_NoOverlap)
{
    test_difference(Set({ 1, 2, 3 }), Set({ 4, 5, 6 }), Set{ 1, 2, 3 }, Set{ 4, 5, 6 });
}

TYPED_TEST(SetsTest, Difference_PartialOverlap)
{
    test_difference(Set({ 1, 2, 3, 4, 5 }), Set({ 3, 4, 5, 6, 8 }), Set{ 1, 2 }, Set{ 6, 8 });
}

TYPED_TEST(SetsTest, Difference_TotalOverlap)
{
    test_difference(Set({ 1, 2, 3, 4, 5 }), Set({ 1, 2, 3, 4, 5 }), Set{}, Set{});
}

TYPED_TEST(SetsTest, Difference_OverlappingVarious)
{
    std::vector<typename Set::SetElement> input = {2, 5, 4, 8, 0, 25};
    Set set(input);
    ASSERT_EQ(set, Set({2, 5, 4, 8, 0, 25}));

    auto inp1 = std::vector<typename Set::SetElement>{1, 2, 3, 4};
    auto dif1 = Set(inp1);
    auto res1 = set.difference(dif1);
    ASSERT_EQ(res1, Set({5, 8, 0, 25}));

    auto inp2 = std::vector<typename Set::SetElement>{22, 44, 5, 11, 8, 51, 0, 25};
    auto dif2 = Set(inp2);
    auto res2 = res1.difference(dif2);
    ASSERT_EQ(res2, Set());
}

TYPED_TEST(SetsTest, Difference_Singleton)
{
    Set set{2, 5, 4, 8, 0, 25};
    ASSERT_EQ(set, Set({2, 5, 4, 8, 0, 25}));
    Set res = set.difference(2)
        .difference(5)
        .difference(4)
        .difference(25)
        .difference(37)
        .difference(58)
        .difference(99)
        .difference(0);
    ASSERT_EQ(res, Set({8}));
}


TYPED_TEST(SetsTest, DifferenceInplace_Empty)
{
    Set set;
    set.difference_inplace(Set());
    ASSERT_EQ(set, Set());
}

TYPED_TEST(SetsTest, DifferenceInplace_Singleton)
{
    Set set {2, 5, 4, 8, 0, 25};
    ASSERT_EQ(set, Set({2, 5, 4, 8, 0, 25}));
    set.difference_inplace(2);
    set.difference_inplace(5);
    set.difference_inplace(4);
    set.difference_inplace(25);
    set.difference_inplace(16);
    set.difference_inplace(38);
    set.difference_inplace(55);
    set.difference_inplace(0);
    ASSERT_EQ(set, Set({8}));
}


TYPED_TEST(SetsTest, Remove_Empty) {
    Set s;
    s.remove(0);
    ASSERT_EQ(s, Set{});
}

TYPED_TEST(SetsTest, Remove_Beginning) {
    Set s{1, 2, 3, 4, 5};
    s.remove(1);
    ASSERT_EQ(s, Set({2, 3, 4, 5}));
}
TYPED_TEST(SetsTest, Remove_Middle) {
    Set s{1, 2, 3, 4, 5};
    s.remove(3);
    ASSERT_EQ(s, Set({1, 2, 4, 5}));
}
TYPED_TEST(SetsTest, Remove_End) {
    Set s{1, 2, 3, 4, 5};
    s.remove(5);
    ASSERT_EQ(s, Set({1, 2, 3, 4}));
}
TYPED_TEST(SetsTest, Remove_NonExistent) {
    Set s{1, 2, 4, 5};
    s.remove(3);
    ASSERT_EQ(s, Set({1, 2, 4, 5}));
}

TYPED_TEST(SetsTest, Add_Empty) {
    Set s;
    s.add(0);
    ASSERT_EQ(s, Set({0}));
}
TYPED_TEST(SetsTest, Add_Beginning) {
    Set s{2, 3, 4, 5};
    s.add(1);
    ASSERT_EQ(s, Set({1, 2, 3, 4, 5}));
}
TYPED_TEST(SetsTest, Add_Middle) {
    Set s{1, 2, 4, 5};
    s.add(3);
    ASSERT_EQ(s, Set({1, 2, 3, 4, 5}));
}
TYPED_TEST(SetsTest, Add_End) {
    Set s{1, 2, 3, 4};
    s.add(5);
    ASSERT_EQ(s, Set({1, 2, 3, 4, 5}));
}


TYPED_TEST(SetsTest, Contains_Empty)
{
    Set set;
    ASSERT_FALSE(set.contains(7));
    ASSERT_FALSE(set.contains(12));
    ASSERT_FALSE(set.contains(88));
    ASSERT_FALSE(set.contains(1));
}

TYPED_TEST(SetsTest, Contains_Various)
{
    Set set{2, 5, 4, 8, 0, 25};
    ASSERT_FALSE(set.contains(7));
    ASSERT_FALSE(set.contains(12));
    ASSERT_FALSE(set.contains(88));
    ASSERT_FALSE(set.contains(1));

    ASSERT_TRUE(set.contains(5));
    ASSERT_TRUE(set.contains(4));
    ASSERT_TRUE(set.contains(2));
    ASSERT_TRUE(set.contains(8));
    ASSERT_TRUE(set.contains(0));
    ASSERT_TRUE(set.contains(25));
}


TYPED_TEST(SetsTest, Range_Empty)
{
    auto set = Set::Range(0);
    ASSERT_EQ(set.cardinality(), 0);
    ASSERT_EQ(set, Set());
}

TYPED_TEST(SetsTest, Range_Various)
{
    auto set1 = Set::Range(3);
    ASSERT_EQ(set1, Set({0, 1, 2}));

    auto set2 = Set::Range(5);
    ASSERT_EQ(set2, Set({0, 1, 2, 3, 4}));
}


TYPED_TEST(SetsTest, ToArray_Empty)
{
    Set empty;
    typename Set::SetElement data[3] = {42, 42, 42};

    empty.toArray(&data[0]);
    ASSERT_EQ(data[0], 42);
    ASSERT_EQ(data[1], 42);
    ASSERT_EQ(data[2], 42);
}

TYPED_TEST(SetsTest, ToArray_Basic)
{
    Set set{2, 4, 5};
    std::vector<typename Set::SetElement> buffer(3, 42);
    set.toArray(buffer.data());
    ASSERT_THAT(buffer, UnorderedElementsAre(4, 2, 5));
}