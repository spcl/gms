#pragma once

#include <vector>
#include <gms/common/types.h>
#include <gms/third_party/roaring/roaring.hh>
#include <type_traits>

/**
 * @brief Set implementation wrapper for Roaring bitmaps.
 *
 * This template class is used for the definition of RoaringSet and Roaring64Set further below.
 *
 * @tparam R The Roaring set container type.
 */
template <class R>
class RoaringSetBase
{
private:
    using RoaringElement = typename std::conditional<std::is_same<R, Roaring>::value, std::uint32_t, std::uint64_t>::type;

    explicit RoaringSetBase(const R &set) : set(set)
    {}

    explicit RoaringSetBase(R &&set) : set(std::move(set))
    {}

public:
    using SetElement = typename std::conditional<std::is_same<R, Roaring>::value, std::int32_t, std::int64_t>::type;

    /**
     * Instantiate an empty set.
     */
    RoaringSetBase() = default;

    RoaringSetBase(RoaringSetBase &&other) noexcept = default;
    RoaringSetBase &operator=(RoaringSetBase &&) = default;

    // Note: Use clone() if you want a copy of a set.
    RoaringSetBase(const RoaringSetBase &) = delete;
    // Note: Use clone() if you want a copy of a set.
    RoaringSetBase &operator=(const RoaringSetBase &) = delete;

    /**
     * @brief Create an instance copying the referenced data.
     *
     * @param start first item of the set
     * @param count number of set elements
     */
    RoaringSetBase(const SetElement *start, size_t count) :
        set(count, reinterpret_cast<const RoaringElement *>(start))
    {
        // TODO(opt) check if it might make sense to expose this method for some algorithms
        set.runOptimize();
    }

    explicit RoaringSetBase(const std::vector<SetElement> &vector) :
        RoaringSetBase(vector.data(), vector.size())
    {}

    // TODO(opt)
    explicit RoaringSetBase(const std::initializer_list<SetElement> &data) :
        RoaringSetBase(std::vector<SetElement>(data))
    {}

    /**
     * Create a set instance containing only the provided element.
     *
     * @param element
     */
    explicit RoaringSetBase(SetElement element) : RoaringSetBase(&element, 1) {}

    RoaringSetBase clone() const
    {
        return RoaringSetBase(set);
    }

    size_t cardinality() const
    {
        return set.cardinality();
    }

    // TODO it might be confusing that one gets unsigned integers from the iterators here
    //  we could fix it by copying RoaringSetBitForwardIterator and changing the uint typedefs,
    //  then return this instead of calling set.begin and set.end() and make sure that in our
    //  code (i.e. this class) we only use RoaringSetBase.begin and end and not on the roaring directly
    /**
     * Returns an iterator to the first element of the set.
     *
     * @return
     */
    typename R::const_iterator begin() const
    {
        return this->set.begin();
    }

    typename R::const_iterator &end() const
    {
        // Note that this returns a special "bogus iterator" which while shared
        // across threads shouldn't cause any issues since it's not bound to a specific
        // instance.
        //
        // For this reason it's also fine to return a reference to the static object,
        // saving unnecessary allocations.
        return set.end();
    }

    RoaringSetBase union_with(const RoaringSetBase &other) const
    {
        return RoaringSetBase(set | other.set);
    }

    RoaringSetBase union_with(const SetElement other) const
    {
        auto temp = clone();
        temp.union_inplace(other);
        return temp;
    }

    void union_inplace(const RoaringSetBase &other)
    {
        set |= other.set;
    }

    void union_inplace(const SetElement other)
    {
        set.add(RoaringElement(other));
    }

    size_t union_count(const RoaringSetBase &other) const
    {
        return set.or_cardinality(other.set);
    }

    RoaringSetBase intersect(const RoaringSetBase &other) const
    {
        return RoaringSetBase(set & other.set);
    }

    void intersect_inplace(const RoaringSetBase &other)
    {
        set &= other.set;
    }

    size_t intersect_count(const RoaringSetBase &other) const
    {
        if constexpr (std::is_same<R, Roaring>::value) {
            return set.and_cardinality(other.set);
        } else {
            auto intersection = intersect(other);
            return intersection.cardinality();
        }
    }

    RoaringSetBase difference(const RoaringSetBase &other) const
    {
        return RoaringSetBase(set - other.set);
    }

    RoaringSetBase difference(const SetElement other) const
    {
        auto temp = clone();
        temp.difference_inplace(other);
        return temp;
    }

    void difference_inplace(const RoaringSetBase &other)
    {
        set -= other.set;
    }

    void difference_inplace(const SetElement other)
    {
        set.remove(RoaringElement(other));
    }

    bool contains(const SetElement x) const
    {
        return set.contains(RoaringElement(x));
    }

    void add(SetElement element)
    {
        set.add(RoaringElement(element));
    }

    void remove(SetElement element)
    {
        set.remove(RoaringElement(element));
    }

    template <class T>
    void toArray(T *array) const
    {
        if constexpr (std::is_same<R, Roaring>::value) {
            static_assert(sizeof(SetElement) == 4, "SetElement should be 32 bit if used with Roaring");
            set.toUint32Array(reinterpret_cast<uint32_t *>(array));
        } else {
            static_assert(std::is_same<R, Roaring64Map>::value, "Only Roaring and Roaring64Map are allowed");
            static_assert(sizeof(SetElement) == 8, "SetElement should be 64 bit if used with Roaring64");
            set.toUint64Array(reinterpret_cast<uint64_t *>(array));
        }
    }

    bool operator==(const RoaringSetBase &other) const
    {
        return set == other.set;
    }

    bool operator!=(const RoaringSetBase &other) const
    {
        return !(*this == other);
    }

    /**
     * Instantiates the set {0, 1, ..., bound - 1}.
     *
     * @param bound
     * @return
     */
    static RoaringSetBase Range(unsigned int bound)
    {
        std::vector<SetElement> temp(bound);
        std::iota(temp.begin(), temp.end(), 0);
        return RoaringSetBase(temp);
    }

private:
    R set;
};

using RoaringSet32 = RoaringSetBase<Roaring>;
using RoaringSet64 = RoaringSetBase<Roaring64Map>;
using RoaringSet = std::conditional<std::is_same<NodeId, int64_t>::value, RoaringSet64, RoaringSet32>::type;

