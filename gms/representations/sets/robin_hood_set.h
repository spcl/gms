#pragma once
#pragma once

#include <vector>
#include <gms/common/types.h>
#include <gms/third_party/robin_hood.h>
#include <type_traits>
#include <cstring>


template <class TSetElement>
class RobinHoodSetBase
{
private:
    using Container = robin_hood::unordered_set<TSetElement>;

    Container set;

    explicit RobinHoodSetBase(const Container &set) : set(set)
    {}

    explicit RobinHoodSetBase(Container &&set) : set(std::move(set))
    {}

public:
    using SetElement = TSetElement;
    
    /**
     * Instantiate an empty set.
     */
    RobinHoodSetBase() = default;

    RobinHoodSetBase(RobinHoodSetBase &&other) noexcept = default;
    RobinHoodSetBase &operator=(RobinHoodSetBase &&) = default;

    // Note: Use clone() if you want a copy of a set.
    RobinHoodSetBase(const RobinHoodSetBase &) = delete;
    // Note: Use clone() if you want a copy of a set.
    RobinHoodSetBase &operator=(const RobinHoodSetBase &) = delete;

    /**
     * @brief Create an instance copying the referenced data.
     *
     * @param start first item of the set
     * @param count number of set elements
     */
    RobinHoodSetBase(const SetElement *start, size_t count) :
            set(start, start + count)
    {
    }

    explicit RobinHoodSetBase(const std::vector<SetElement> &vector) :
            RobinHoodSetBase(vector.data(), vector.size())
    {}

    // TODO(opt)
    explicit RobinHoodSetBase(const std::initializer_list<SetElement> &data) :
            RobinHoodSetBase(std::vector<SetElement>(data))
    {}

    /**
     * Create a set instance containing only the provided element.
     *
     * @param element
     */
    explicit RobinHoodSetBase(SetElement element) : RobinHoodSetBase(&element, 1) {}

    RobinHoodSetBase clone() const
    {
        return RobinHoodSetBase(set);
    }

    size_t cardinality() const
    {
        return set.size();
    }

    /**
     * Returns an iterator to the first element of the set.
     *
     * @return
     */
    typename Container::const_iterator begin() const
    {
        return set.cbegin();
    }

    typename Container::const_iterator end() const
    {
        return set.cend();
    }

    RobinHoodSetBase union_with(const RobinHoodSetBase &other) const
    {
        auto result = clone();
        result.union_inplace(other);
        return result;
    }

    RobinHoodSetBase union_with(const SetElement other)
    {
        auto result = clone();
        result.union_inplace(other);
        return result;
    }

    void union_inplace(const RobinHoodSetBase &other)
    {
        set.insert(other.begin(), other.end());
    }

    void union_inplace(const SetElement other)
    {
        set.insert(other);
    }

    size_t union_count(const RobinHoodSetBase &other) const
    {
        // TODO opt
        return union_with(other).cardinality();
    }

    RobinHoodSetBase intersect(const RobinHoodSetBase &other) const
    {
        bool should_swap = cardinality() > other.cardinality();

        Container result = should_swap ? other.set : set;

        if (!should_swap) {
            for (SetElement el : *this) {
                if (!other.contains(el)) {
                    result.erase(el);
                }
            }
        } else {
            for (SetElement el : other) {
                if (!contains(el)) {
                    result.erase(el);
                }
            }
        }

        return RobinHoodSetBase(result);
    }

    void intersect_inplace(const RobinHoodSetBase &other)
    {
        // TODO(opt) maybe it's not necessary to clone? is deleting during iteration allowed?
        auto result = intersect(other);
        set = std::move(result.set);
    }

    size_t intersect_count(const RobinHoodSetBase &other) const
    {
        bool q = cardinality() >= other.cardinality();
        const Container &minor = q ? other.set : set;
        const Container &major = q ? set : other.set;

        size_t count = 0;
        for (SetElement e : minor) {
            if (major.find(e) != major.end()) {
                ++count;
            }
        }

        return count;
    }

    RobinHoodSetBase difference(const RobinHoodSetBase &other) const
    {
        auto result = clone();
        result.difference_inplace(other);
        return result;
    }

    RobinHoodSetBase difference(const SetElement other)
    {
        auto result = clone();
        result.difference_inplace(other);
        return result;
    }

    void difference_inplace(const RobinHoodSetBase &other)
    {
        for (SetElement el : other) {
            set.erase(el);
        }
        //set.erase(other.begin(), other.end());
    }

    void difference_inplace(const SetElement element)
    {
        set.erase(element);
    }

    bool contains(const SetElement x) const
    {
        return set.contains(x);
    }

    void add(SetElement element)
    {
        set.insert(element);
    }

    void remove(SetElement element)
    {
        set.erase(element);
    }

    template <class T>
    void toArray(T *array) const
    {
        // TODO(opt)
        size_t pos = 0;
        for (SetElement el : set) {
            array[pos++] = el;
        }
        //std::memcpy(array, set.begin(), set.size() * sizeof(SetElement));
    }

    bool operator==(const RobinHoodSetBase &other) const
    {
        return set == other.set;
    }

    bool operator!=(const RobinHoodSetBase &other) const
    {
        return !(*this == other);
    }

    /**
     * Instantiates the set {0, 1, ..., bound - 1}.
     *
     * @param bound
     * @return
     */
    static RobinHoodSetBase Range(unsigned int bound)
    {
        std::vector<SetElement> temp(bound);
        std::iota(temp.begin(), temp.end(), 0);
        return RobinHoodSetBase(temp);
    }
};

using RobinHoodSet = RobinHoodSetBase<NodeId>;
using RobinHoodSet32 = RobinHoodSetBase<int32_t>;
using RobinHoodSet64 = RobinHoodSetBase<int64_t>;
