#ifndef PPLIB_UTIL_ORDEREDCOLLECTION_H
#define PPLIB_UTIL_ORDEREDCOLLECTION_H

#include <iostream>
#include <string>

#include "auxiliary.h"

namespace coreOrdering
{
    template<typename Key_T, typename Value_T>
    class OrderedCollection
    {
    public:
        typedef KeyValuePair<Key_T, Value_T> KeyValue_T;

        virtual KeyValue_T GetKeyValue(const Key_T& key) const = 0;
        virtual KeyValue_T& GetKeyValueRef(const Key_T& key) = 0;
        virtual int GetIndex(const Key_T& key) const = 0;
        virtual void DecreaseValueOfKey(const Key_T& key) = 0;
        virtual uint Size() const = 0;
        virtual KeyValue_T PopHead() = 0;
    };

    template<typename Key_T, typename Value_T>
    class TrackingBubblingArray : public OrderedCollection<
        Key_T, Value_T>
    {
    public:
        typedef KeyValuePair<Key_T, Value_T> KeyValue_T;
        typedef NodeComparerMax Comparer_T;

    private:
        uint _size;
        uint _capacity;
        const Comparer_T* _cmp;
        KeyValue_T* _keyValue;
        uint _start;
        int* _keyLocation;
        
        
        bool _cleanData;

        inline void SwapKeys(const Key_T& keyA, const Key_T& keyB)
        {
            int idxA = _keyLocation[keyA];
            int idxB = _keyLocation[keyB];

            std::swap(_keyValue[idxA], _keyValue[idxB]);
            std::swap(_keyLocation[keyA], _keyLocation[keyB]);
        }

        inline void Swap(const int idxA, const int idxB)
        {
            Key_T keyA = _keyValue[idxA].Key;
            Key_T keyB = _keyValue[idxB].Key;

            std::swap(_keyValue[idxA], _keyValue[idxB]);
            std::swap(_keyLocation[keyA], _keyLocation[keyB]);
        }

        inline void BubbleUp(int idx)
        {
            int prev = idx-1;
            while(idx>(long int)_start)
            {
                if(_cmp->cmp(_keyValue[idx], _keyValue[prev]))
                {
                    Swap(prev,idx);
                    idx = prev;
                    prev = idx-1;
                }
                else break;
            }
        }

    public:
        TrackingBubblingArray(KeyValue_T* keyValues, uint size)
        : _size(size), _capacity(size), _cmp(new Comparer_T()), _keyValue(keyValues), _start(0)
        {
            std::sort(_keyValue, _keyValue+_size, *_cmp);
            _keyLocation = new int[_capacity];
            for(int i = 0; i < (long int)_size; i++)
            {
                _keyLocation[_keyValue[i].Key] = i;
            }

        }

        ~TrackingBubblingArray()
        {
            delete _cmp;
            delete[] _keyLocation;
        }

        virtual int GetIndex(const Key_T& key) const override 
        {
            return _keyLocation[key];
        }

        virtual KeyValue_T GetKeyValue(const Key_T& key) const override
        {
            return _keyValue[GetIndex(key)];
        }

        virtual KeyValue_T& GetKeyValueRef(const Key_T& key) override
        {
            return _keyValue[GetIndex(key)];
        }

        virtual void DecreaseValueOfKey(const Key_T& key) override 
        {
            int idx = GetIndex(key);
            _keyValue[idx].Value--;

            BubbleUp(idx);
        }

        virtual uint Size() const override 
        {
            return _size;
        }

        virtual KeyValue_T PopHead() override 
        {
            KeyValue_T head = _keyValue[_start];
            _keyLocation[head.Key] = -1;
            _start++;
            _size--;
            return head;
        }
    };

    template<
        typename Key_T, typename Value_T>
    class TrackingStdHeap : public OrderedCollection<
        Key_T, Value_T>
    {
    public:
        typedef KeyValuePair<Key_T, Value_T> KeyValue_T;
        typedef NodeComparerMin Comparer_T;

    private:
        KeyValue_T* _keyValue;
        uint _size;
        uint _capacity;
        int* _keyLocation;
        const Comparer_T* _cmp;
        bool _cleanData;

        inline void SwapKeys(const Key_T& keyA, const Key_T& keyB)
        {
            int idxA = _keyLocation[keyA];
            int idxB = _keyLocation[keyB];

            std::swap(_keyValue[idxA], _keyValue[idxB]);
            std::swap(_keyLocation[keyA], _keyLocation[keyB]);
        }

        inline void Swap(const int idxA, const int idxB)
        {
            Key_T keyA = _keyValue[idxA].Key;
            Key_T keyB = _keyValue[idxB].Key;

            std::swap(_keyValue[idxA], _keyValue[idxB]);
            std::swap(_keyLocation[keyA], _keyLocation[keyB]);
        }

        inline void BubbleUp(int idx)
        {
            uint parent = (idx-1)/2;
            while(idx > 0)
            {
                if(_cmp->cmp(_keyValue[parent], _keyValue[idx]))
                {
                    Swap(parent, idx);
                    idx = parent;
                    parent = (idx-1)/2;
                }
                else break;
            }
        }

        inline void BubbleDown()
        {
            uint i=0, j1=1, j2=2, j;
            while( j1 < Size())
            {
                j = (
                    (j2 < Size()) && _cmp->cmp(_keyValue[j1], _keyValue[j2])
                ) ? j2 : j1;
                if(_cmp->cmp(_keyValue[i], _keyValue[j]))
                {
                    Swap(i,j);
                    i = j;
                    j1 = 2*i+1;
                    j2 = 2*i+2;
                } 
                else break;
            }
        }

    public:

        TrackingStdHeap(uint capacity)
        : _keyValue(new KeyValue_T[capacity]), _keyLocation(new int[capacity]),
          _size(0), _capacity(capacity), _cmp(new Comparer_T()), _cleanData(true)
        {
            for(uint i = 0; i < _capacity; i++)
            {
                _keyLocation[i] = -1;
            }
        }

        TrackingStdHeap(KeyValue_T* data, uint size)
        : _keyValue(data), _size(size), _capacity(size), _cmp(new Comparer_T()), 
          _cleanData(false)
        {
            std::make_heap(_keyValue, _keyValue+_size, *_cmp);
            _keyLocation = new int[_size];
            for(int i = 0; i < (long int)_size; i++)
            {
                _keyLocation[_keyValue[i].Key] = i;
            }
        }

        ~TrackingStdHeap() 
        {
            delete _cmp;
            delete[] _keyLocation;
            if(_cleanData)
            {
                delete[] _keyValue;
            }
        }

        virtual KeyValue_T GetKeyValue(const Key_T& key) const override 
        {
            return _keyValue[_keyLocation[key]];
        }

        virtual KeyValue_T& GetKeyValueRef(const Key_T& key) override 
        {
            return _keyValue[_keyLocation[key]];
        }

        virtual int GetIndex(const Key_T& key) const override
        {
            return _keyLocation[key];
        }

        virtual void DecreaseValueOfKey(const Key_T& key) override 
        {
            int idx = GetIndex(key);
            if(idx != -1)
            {
                _keyValue[idx].Value--;
                BubbleUp(idx);
            }
        }

        virtual uint Size() const override 
        {
            return _size;
        }

        virtual KeyValue_T PopHead() override 
        {
            KeyValue_T head = _keyValue[0];
            Swap(0, --_size);
            BubbleDown();
            _keyLocation[head.Key] = -1;

            return head;
        }
    };

} // end namespace

#endif