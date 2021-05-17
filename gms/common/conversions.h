#pragma once

#include <cstdint>
#include <vector>

namespace gms {

    template<typename Set>
    std::vector<uint32_t> setToVector(Set &set) {
        auto size = set.cardinality();
        std::vector<uint32_t> res(size);
        int counter = 0;
        for (auto v : set)
            res[counter++] = v;

        return res;
    }

    template<typename Set>
    std::vector<std::vector<uint32_t>> setsToVector(std::vector<Set> &set) {
        auto size = set.size();
        std::vector<std::vector<uint32_t>> res(size);
        int counter = 0;
        for (auto &inner : set) {
            res[counter++] = setToVector(inner);
        }

        return res;
    }

} // namespace gms