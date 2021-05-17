#pragma once

#include <vector>
#include <algorithm>

using SortedSetContainer = std::vector<NodeId>;

template <bool HasDuplicates = true, typename Iterator, typename Element>
inline void skip_while_equal(Iterator &iterator, Iterator end, Element element)
{
    if (!HasDuplicates)
    {
        ++iterator;
    }
    else
    {
        while (iterator != end)
        {
            if (*iterator == element)
            {
                ++iterator;
            }
            else
                break;
        }
    }
}

template <class Container, typename IterL, typename IterR>
inline Container vec_set_union(IterL lstart, IterL lend, IterR rstart, IterR rend)
{
    Container container;
    std::set_union(lstart, lend, rstart, rend, std::back_inserter(container));
    return container;
}
template <class Container, typename IterL, typename IterR>
inline Container vec_set_intersect(IterL lstart, IterL lend, IterR rstart, IterR rend)
{
    Container container;
    std::set_intersection(lstart, lend, rstart, rend, std::back_inserter(container));
    return container;
}

template <typename IterL, typename IterR>
inline size_t vec_set_intersect_count(IterL lstart, IterL lend, IterR rstart, IterR rend)
{
    size_t count = 0;
    while (lstart != lend && rstart != rend)
    {
        auto value_a = *lstart;
        auto value_b = *rstart;

        if (value_a == value_b)
        {
            count++;
            skip_while_equal<false>(lstart, lend, value_a);
            skip_while_equal<false>(rstart, rend, value_b);
        }
        else
        {
            if (value_a > value_b)
            {
                skip_while_equal<false>(rstart, rend, value_b);
            }
            else
                skip_while_equal<false>(lstart, lend, value_a);
        }
    }

    return count;
}

template <class Container, typename IterL, typename IterR>
inline Container vec_set_difference(IterL lstart, IterL lend, IterR rstart, IterR rend)
{
    Container container;
    while (lstart != lend && rstart != rend)
    {
        auto value_a = *lstart;
        auto value_b = *rstart;

        if (value_a == value_b)
        {
            skip_while_equal<false>(lstart, lend, value_a);
            skip_while_equal<false>(rstart, rend, value_b);
        }
        else
        {
            if (value_a > value_b)
            {
                skip_while_equal<false>(rstart, rend, value_b);
            }
            else
            {
                container.push_back(value_a);
                skip_while_equal<false>(lstart, lend, value_a);
            }
        }
    }

    if (lstart != lend)
    {
        container.insert(container.end(), lstart, lend);
    }
    return container;
}