#pragma once

#include <algorithm>
#include <vector>
#include <cassert>
#include <cstring>
#include <numeric>

#include <gms/common/types.h>

#include "sorted_set_operations.h"

/**
 * @brief Set implementation based on a sorted vector.
 *
 * All operations will respect the sorted invariant, but you have to make sure that the set will remain sorted
 * if you modify it in other ways.
 *
 * Unlike SortedSetRef this class holds a copy of the data and doesn't sort potential input data.
 */
template <class TSetElement>
class SortedSetBase
{
public:
    using SetElement = TSetElement;
    using Container = std::vector<TSetElement>;

    /**
     * Instantiate an empty set.
     */
    SortedSetBase() = default;

    SortedSetBase(SortedSetBase &&other) noexcept = default;
    SortedSetBase &operator=(SortedSetBase &&) = default;

    // Note: Use clone() if you want a copy of a set.
    SortedSetBase(const SortedSetBase &) = delete;
    // Note: Use clone() if you want a copy of a set.
    SortedSetBase &operator=(const SortedSetBase &) = delete;

    /**
     * Instantiate from an existing vector.
     * This method isn't part of the set interface.
     *
     * @param _data
     * @param is_sorted true if the elements are guaranteed to be sorted,
     *                  false otherwise (and if unknown)
     */
    SortedSetBase(Container &&_data, bool is_sorted) : data(std::move(_data))
    {
        if (!is_sorted) {
            std::sort(data.begin(), data.end());
        } else {
            assert(std::is_sorted(data.begin(), data.end()));
        }
    }

    /**
     * @brief Create an instance copying and sorting the referenced data.
     *
     * @param start first item of the set
     * @param count number of set elements
     */
    SortedSetBase(const SetElement *start, size_t count) :
        SortedSetBase(Container(start, start + count), false)
    {}

    explicit SortedSetBase(const std::vector<SetElement> &data) :
        SortedSetBase(data.data(), data.size())
    {}

    explicit SortedSetBase(const std::initializer_list<SetElement> &data) :
        SortedSetBase(Container(data), false)
    {}

    /**
     * Create a set instance containing only the provided element.
     *
     * @param element
     */
    explicit SortedSetBase(SetElement element) : data(1, element)
    {}

    SortedSetBase clone() const
    {
        return SortedSetBase(Container(data), true);
    }

    size_t cardinality() const
    {
        return data.size();
    }

    typename Container::const_iterator begin() const
    {
        return data.cbegin();
    }

    typename Container::const_iterator end() const
    {
        return data.cend();
    }

    SortedSetBase union_with(const SortedSetBase &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return SortedSetBase(vec_set_union<Container>(this->begin(), this->end(), set.begin(), set.end()), true);
    }

    SortedSetBase union_with(SetElement element) const
    {
        // TODO (optimize but this isn't used anywhere yet)
        check_is_sorted();
        auto result = clone();
        result.union_inplace(element);
        return result;
    }

    void union_inplace(const SortedSetBase &other)
    {
        check_is_sorted();
        other.check_is_sorted();
        // TODO
        data = vec_set_union<Container>(begin(), end(), other.begin(), other.end());
    }

    void union_inplace(SetElement element)
    {
        auto it = std::lower_bound(begin(), end(), element);
        if (it != end() && *it == element)
            return;

        auto index = it - begin();
        data.resize(data.size() + 1);
        std::copy_backward(data.begin() + index, data.end() - 1, data.end());
        *(data.begin() + index) = element;
    }

    size_t union_count(const SortedSetBase &other) const {
        size_t count = 0;
        auto it0 = begin();
        auto it1 = other.begin();
        while (it0 != end() && it1 != other.end()) {
            count++;
            if (*it0 == *it1) {
                ++it0;
                ++it1;
            } else if (*it0 > *it1) {
                ++it1;
            } else {
                ++it0;
            }
        }
        count += std::distance(it0, end());
        count += std::distance(it1, other.end());
        return count;
    }

    template <typename Set>
    SortedSetBase intersect(const Set &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return SortedSetBase(vec_set_intersect<Container>(this->begin(), this->end(), set.begin(), set.end()), true);
    }

    template <class Set>
    void intersect_inplace(const Set &other) {
        check_is_sorted();
        other.check_is_sorted();
        // TODO
        data = vec_set_intersect<Container>(begin(), end(), other.begin(), other.end());
    }

    template <typename Set>
    size_t intersect_count(const Set &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return vec_set_intersect_count(this->begin(), this->end(), set.begin(), set.end());
    }

    SortedSetBase difference(const SortedSetBase &set) const
    {
        this->check_is_sorted();
        set.check_is_sorted();
        return SortedSetBase(vec_set_difference<Container>(this->begin(), this->end(), set.begin(), set.end()), true);
    }

    SortedSetBase difference(SetElement element) const
    {
        this->check_is_sorted();
        auto set = clone();
        set.difference_inplace(element);
        return set;
    }

    void difference_inplace(const SortedSetBase &set)
    {
        this->check_is_sorted();
        set.check_is_sorted();
        // TODO this could be optimized since in some cases it might turn out faster to not allocate one more vector,
        //      but instead shrink its contents (which would require to move everything thus it could be costly again)
        data = vec_set_difference<Container>(this->begin(), this->end(), set.begin(), set.end());
    }

    void difference_inplace(SetElement element) {
        this->check_is_sorted();
        auto position = std::lower_bound(begin(), end(), element);
        if (position != end() && *position == element) {
            data.erase(position);
            // Check that it wasn't actually a multiset.
            assert(std::find(begin(), end(), element) == end());
        }
    }

    bool contains(const SetElement x) const
    {
        auto it = std::lower_bound(begin(), end(), x);
        return !(it == end() || *it != x);
    }

    void add(SetElement element)
    {
        union_inplace(element);
    }

    void remove(SetElement element)
    {
        difference_inplace(element);
    }

    void toArray(SetElement *array) const
    {
        if (cardinality() > 0) {
            std::memcpy(array, data.data(), data.size() * sizeof(SetElement));
        }
    }

    bool operator==(const SortedSetBase &other) const
    {
        return data == other.data;
    }

    bool operator!=(const SortedSetBase &other) const
    {
        return data != other.data;
    }

    /**
     * Instantiates the set {0, 1, ..., bound - 1}.
     *
     * @param bound
     * @return
     */
    static SortedSetBase Range(unsigned int bound)
    {
        std::vector<SetElement> temp(bound);
        std::iota(temp.begin(), temp.end(), 0);
        return SortedSetBase(std::move(temp), true);
    }

    // TODO should be private but is currently used in assertions in set operations
    void check_is_sorted() const
    {
        assert(std::is_sorted(data.begin(), data.end()));
    }

private:
    Container data;
};

using SortedSet = SortedSetBase<NodeId>;
using SortedSet32 = SortedSetBase<int32_t>;
using SortedSet64 = SortedSetBase<int64_t>;