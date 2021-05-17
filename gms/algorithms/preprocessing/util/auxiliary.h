#ifndef PPLIB_UTIL_AUXILIARY_H
#define PPLIB_UTIL_AUXILIARY_H

#include <iostream>
#include <string>

namespace coreOrdering
{
    template<typename Key_T, typename Value_T>
    struct KeyValuePair
    {
        Key_T Key;
        Value_T Value;

        KeyValuePair(Key_T key, Value_T value)
        : Key(key), Value(value) {}

        KeyValuePair() : Key(0), Value(0) {}

        bool operator==(const KeyValuePair<Key_T, Value_T>& b) const
        {
            return Key == b.Key && Value == b.Value;
        }

        bool operator!=(const KeyValuePair<Key_T, Value_T>& b) const
        {
            return !operator==(b);
        }
    };

    template<typename Key_T, typename Value_T>
    std::ostream& operator<<(std::ostream& os, const KeyValuePair<Key_T, Value_T>& kv)
    {
        os << "(" << kv.Key << "/" << kv.Value << ")";
        return os;
    }

    // comparer class to build a min heap
    class NodeComparerMin
    {
        // comparer according to
        // https://en.cppreference.com/w/cpp/named_req/Compare
    public:
        NodeComparerMin() {}

        // comparison function to build min-heap
        template<typename a_t, typename b_t>
        bool cmp(const a_t& a, const b_t& b) const
        {
            return a.Value > b.Value;
        }

        template<typename a_t, typename b_t>
        bool operator()(const a_t& a, const b_t& b) const 
        {
            return cmp(a, b);
        }

        template<typename a_t, typename b_t>
        bool equiv(const a_t& a, const b_t& b) const
        {
            return a.Value == b.Value;
        }
    };

    class NodeComparerMax
    {
    public:
        NodeComparerMax() {}

        template<typename a_t, typename b_t>
        bool cmp(const a_t& a, const b_t& b) const
        {
            return a.Value < b.Value;
        }

        template<typename a_t, typename b_t>
        bool operator()(const a_t& a, const b_t& b) const 
        {
            return cmp(a,b);
        }

        template<typename a_t, typename b_t>
        bool equiv(const a_t& a, const b_t& b) const 
        {
            return a.Value == b.Value;
        }
    };
} // end namespace

#endif