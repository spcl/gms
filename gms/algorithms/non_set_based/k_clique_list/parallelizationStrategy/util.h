#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gms/common/types.h>

namespace GMS::KClique::Builders
{
    struct NodeHash
    {
        size_t operator()(const NodeId& node) const
        {
            return (size_t)node;
        }
    };

    template<typename Hash_T>
    class Mapping
    {
        const std::unordered_map<NodeId, NodeId, Hash_T>& _map_down;
        const std::vector<NodeId>& _map_up;

    public:
        Mapping(const std::unordered_map<NodeId, NodeId, Hash_T>& map_down,
                const std::vector<NodeId>& map_up)
        : _map_down(map_down), _map_up(map_up)
        {}

        ~Mapping(){}

        NodeId OrigIndex(NodeId idx) const
        {
            if (idx >= _map_up.size()) return -1;
            return _map_up[idx];
        }

        NodeId NewIndex(NodeId idx) const
        {
            const auto kv = _map_down.find(idx);
            if(kv != _map_down.end())
            {
                return kv->second;
            }
            return -1;
        }
    };

    template<typename SetElement_T>
    class SimpleMapping
    {
        pvector<SetElement_T> _oldIdx;
        pvector<SetElement_T> _newIdx;
        uint _size;
    public:
        SimpleMapping(uint coreNumber, uint nrPoints)
        : _oldIdx(pvector<SetElement_T>(coreNumber)), _newIdx(pvector<SetElement_T>(nrPoints, -1)), _size(0)
        {}

        SimpleMapping(const SimpleMapping& other)
        : _size(other._size)
        {
            _oldIdx = pvector<SetElement_T>(other._oldIdx.size());
            for(uint i = 0; i < _oldIdx.size(); i++)
            {
                _oldIdx[i] = other._oldIdx[i];
            }
            _newIdx = pvector<SetElement_T>(other._newIdx.size());
            for(uint i = 0; i < _newIdx.size(); i++)
            {
                _newIdx[i] = other._newIdx[i];
            }
        }

        void Clear()
        {
            for (uint i = 0; i < _size; i++)
            {
                const SetElement_T index = _oldIdx[i];
                _newIdx[index] = -1;
            }
            _size = 0;
        }

        bool AlreadyMapped(SetElement_T oldIdx) const
        {
            return _newIdx[oldIdx] != (SetElement_T)-1;
        }

        SetElement_T MapNode(SetElement_T oldIdx)
        {
            _newIdx[oldIdx] = _size;
            _oldIdx[_size] = oldIdx;
            return _size++;
        }

        SetElement_T MapNodeSafely(SetElement_T oldIdx)
        {
            if(AlreadyMapped(oldIdx))
            {
                return NewIndex(oldIdx);
            }
            return MapNode(oldIdx);
        }

        SetElement_T NewIndex(SetElement_T oldIdx) const
        {
            return _newIdx[oldIdx];
        }

        SetElement_T OrigIndex(SetElement_T newIdx) const
        {
            return newIdx < _size ? _oldIdx[newIdx] : -1;
        }

        typename pvector<SetElement_T>::iterator begin() const
        {
            return _oldIdx.begin();
        }

        typename pvector<SetElement_T>::iterator end() const
        {
            return _oldIdx.begin() + _size;
        }
    };

}