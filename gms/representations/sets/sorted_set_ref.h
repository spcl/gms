#pragma once

#include <gms/common/types.h>

#include "sorted_set_operations.h"

// TODO
// Borrowed data
template <class TSetElement>
class SortedSetRefBase
{
public:
    using SetElement = TSetElement;
    using Container = std::vector<TSetElement>;

    // TODO document somewhere how data is owned by the CSRGraph instance and not our SetGraphs.
    SortedSetRefBase(SetElement *start, size_t count) : data(start), count(count)
    {
        std::sort(start, start + count);
    }

    size_t cardinality() const
    {
        return this->count;
    }

    const SetElement *begin() const
    {
        return this->data;
    }
    const SetElement *end() const
    {
        return this->data + this->count;
    }

    template <typename Set>
    SortedSet union_with(const Set &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return SortedSet(vec_set_union<Container>(this->begin(), this->end(), set.begin(), set.end()), true);
    }
    template <typename Set>
    SortedSet intersect(const Set &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return SortedSet(vec_set_intersect<Container>(this->begin(), this->end(), set.begin(), set.end()), true);
    }
    template <typename Set>
    size_t intersect_count(const Set &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return vec_set_intersect_count(this->begin(), this->end(), set.begin(), set.end());
    }
    template <typename Set>
    SortedSet difference(const Set &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return SortedSet(vec_set_difference<Container>(this->begin(), this->end(), set.begin(), set.end()), true);
    }

    void check_is_sorted() const
    {
        assert(std::is_sorted(this->data, this->data + count));
    }

    bool contains(const SetElement x) const
    {
        return std::lower_bound(begin(), end(), x) != end();
    }

private:
    SetElement *data;
    size_t count;
};

using SortedSetRef = SortedSetRefBase<NodeId>;
using SortedSetRef32 = SortedSetRefBase<int32_t>;
using SortedSetRef64 = SortedSetRefBase<int64_t>;