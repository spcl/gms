#ifndef RAMDOM_SELECT_H_
#define RAMDOM_SELECT_H_

#include <iterator>
#include <random>

namespace GMS::Coloring {

// https://gist.github.com/cbsmith/5538174
template <typename RandomGenerator = std::default_random_engine>
struct random_selector {
    // On most platforms, you probably want to use std::random_device("/dev/urandom")()
    random_selector(RandomGenerator g = RandomGenerator(std::random_device()())) : gen(g) {
        unif_0_1 = std::uniform_real_distribution<double>(0,1);
    }

    double rand_0_1() {
        return unif_0_1(gen);
    }

    int32_t select_num(int start, int end) {
        std::uniform_int_distribution<> dis(start, end);
        return dis(gen);
    }

    template <typename Iter>
    Iter select(Iter start, Iter end) {
        std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
        std::advance(start, dis(gen));
        return start;
    }

    int32_t* select_array(int32_t* array_begin, size_t range) {
        std::uniform_int_distribution<> dis(0, range - 1);
        return array_begin + dis(gen);
    }

    // convenience function
    template <typename Iter>
    Iter operator()(Iter start, Iter end) {
        return select(start, end);
    }

    // convenience function that works on anything with a sensible begin() and end(), and returns with a ref to the
    // value type
    template <typename Container>
    auto operator()(const Container& c) -> decltype(*begin(c))& {
        return *select(begin(c), end(c));
    }

private:
    RandomGenerator gen;
    std::uniform_real_distribution<double> unif_0_1;
};

} // namespace GMS::Coloring

#endif // RANDOM_SELECT_H_
