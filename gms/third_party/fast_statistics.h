#pragma once

#ifndef FASTSTATISTIC_H
#define FASTSTATISTIC_H

#include <stdint.h>
#include <random>
#include <chrono>

#define MERGE(x, y) ((x & 0xFFFFFFF0) | (y))
/*
This code was adapted by Zur Shmaria in order to use parameterized seed (Needed for multithreading)
*/


namespace SplitMix64
{
/* Modified by D. Lemire, August 2017 */
/***
Fast Splittable Pseudorandom Number Generators
Steele Jr, Guy L., Doug Lea, and Christine H. Flood. "Fast splittable
pseudorandom number generators."
ACM SIGPLAN Notices 49.10 (2014): 453-472.
***/

/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)
To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.
See <http://creativecommons.org/publicdomain/zero/1.0/>. */

// original documentation by Vigna:
/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html
   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state; otherwise, we
   rather suggest to use a xoroshiro128+ (for moderately parallel
   computations) or xorshift1024* (for massively parallel computations)
   generator. */

// state for splitmix64
uint64_t state; /* The state can be seeded with any value. */
#pragma omp threadprivate(state)

// call this one before calling splitmix64
static inline void seed(uint64_t seed) { state = seed; }

static inline uint64_t genSeed(int threadID)
{
    state = MERGE(std::chrono::high_resolution_clock::now().time_since_epoch().count(), threadID + 1);
    return state;
}

// returns random number, modifies splitmix64_x
// compared with D. Lemire against
// http://grepcode.com/file/repository.grepcode.com/java/root/jdk/openjdk/8-b132/java/util/SplittableRandom.java#SplittableRandom.0gamma
static inline uint64_t next_r(uint64_t *seed)
{
    uint64_t z = (*seed += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

// same as splitmix64, but does not change the state, designed by D. Lemire
static inline uint64_t next()
{
    return next_r(&state);
}
static inline uint32_t next32() { return (uint32_t)next_r(&state); }
} // namespace SplitMix64

namespace WyRand
{
// adapted to this project by D. Lemire, from https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
// This uses mum hashing.

// state for wyrand
uint64_t state; /* The state can be seeded with any value. */
#pragma omp threadprivate(state)

// call wyrand_seed before calling wyrand
static inline void seed(uint64_t seed) { state = seed; }

static inline uint64_t genSeed(int threadID)
{
    state = MERGE(std::chrono::high_resolution_clock::now().time_since_epoch().count(), threadID + 1);
    return state;
}

static inline uint64_t next_r(uint64_t *s)
{
    *s += UINT64_C(0xa0761d6478bd642f);
    __uint128_t t = (__uint128_t)*s * (*s ^ UINT64_C(0xe7037ed1a0b428db));
    return (t >> 64) ^ t;
}

// returns random number, modifies state
static inline uint64_t next() { return next_r(&state); }
static inline uint32_t next32() { return (uint32_t)next_r(&state); }

} // namespace WyRand

namespace WyHash
{
// adapted to this project by D. Lemire, from https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
// This uses mum hashing.
// state for wyrand
// state for wyhash64
uint64_t state; /* The state can be seeded with any value. */
#pragma omp threadprivate(state)

// call wyhash64_seed before calling wyhash64
static inline void seed(uint64_t seed) { state = seed; }

static inline uint64_t genSeed(int threadID)
{
    state = MERGE(std::chrono::high_resolution_clock::now().time_since_epoch().count(), threadID + 1);
    return state;
}

static inline uint64_t next_r(uint64_t *seed)
{
    *seed += UINT64_C(0x60bee2bee120fc15);
    __uint128_t tmp;
    tmp = (__uint128_t)*seed * UINT64_C(0xa3b195354a39b70d);
    uint64_t m1 = (tmp >> 64) ^ tmp;
    tmp = (__uint128_t)m1 * UINT64_C(0x1b03738712fad5c9);
    uint64_t m2 = (tmp >> 64) ^ tmp;
    return m2;
}

// returns random number, modifies state
static inline uint64_t next() { return next_r(&state); }
static inline uint32_t next32() { return (uint32_t)next_r(&state); }

} // namespace WyHash

namespace Xoroshiro128P
{
// original documentation by Vigna:
/* This is the successor to xorshift128+. It is the fastest full-period
   generator passing BigCrush without systematic failures, but due to the
   relatively short period it is acceptable only for applications with a
   mild amount of parallelism; otherwise, use a xorshift1024* generator.
   Beside passing BigCrush, this generator passes the PractRand test suite
   up to (and included) 16TB, with the exception of binary rank tests,
   which fail due to the lowest bit being an LFSR; all other bits pass all
   tests. We suggest to use a sign test to extract a random Boolean value.
   Note that the generator uses a simulated rotate operation, which most C
   compilers will turn into a single instruction. In Java, you can use
   Long.rotateLeft(). In languages that do not make low-level rotation
   instructions accessible xorshift128+ could be faster.
   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

// state for xoroshiro128plus
uint64_t state[2];
#pragma omp threadprivate(state)

static inline uint64_t rotl(const uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

// call this one before calling xoroshiro128plus
static inline void seed(uint64_t seed)
{
    state[0] = SplitMix64::next_r(&seed);
    state[1] = SplitMix64::next_r(&seed);
}

static inline uint64_t genSeed(int threadID)
{
    seed(MERGE(std::chrono::high_resolution_clock::now().time_since_epoch().count(), threadID + 1));
    return state[0];
}

// returns random number, modifies xoroshiro128plus_s
static inline uint64_t next_r(uint64_t seed[2])
{
    const uint64_t s0 = seed[0];
    uint64_t s1 = seed[1];
    const uint64_t result = s0 + s1;

    s1 ^= s0;
    seed[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14); // a, b
    seed[1] = rotl(s1, 36);                   // c

    return result;
}

static inline uint64_t next()
{
    return next_r(state);
}
static inline uint32_t next32() { return (uint32_t)next_r(state); }

} // namespace Xoroshiro128P

namespace Xoroshiro128PP
{
static inline uint32_t rotl(const uint32_t x, int k)
{
    return (x << k) | (x >> (32 - k));
}

uint32_t state[4];
#pragma omp threadprivate(state)

static inline void seed(uint64_t seed)
{
    state[0] = SplitMix64::next_r(&seed);
    state[1] = SplitMix64::next_r(&seed);
    state[2] = SplitMix64::next_r(&seed);
    state[3] = SplitMix64::next_r(&seed);
}

static inline uint64_t genSeed(int threadID)
{
    seed(MERGE(std::chrono::high_resolution_clock::now().time_since_epoch().count(), threadID + 1));
    return state[0];
}

uint32_t next_r(uint32_t seed[4])
{
    const uint32_t result = rotl(seed[0] + seed[3], 7) + seed[0];

    const uint32_t t = seed[1] << 9;

    seed[2] ^= seed[0];
    seed[3] ^= seed[1];
    seed[1] ^= seed[2];
    seed[0] ^= seed[3];

    seed[2] ^= t;

    seed[3] = rotl(seed[3], 11);

    return result;
}

uint32_t next()
{
    return next_r(state);
}

} // namespace Xoroshiro128PP

namespace GMS::FastStatistics {

/**
 * Provides STL compatible RNG over WyRand.
 */
class WyRandRng {
private:
    uint64_t state;
public:
    using result_type = uint64_t;
    constexpr result_type min() { return 0; }
    constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    WyRandRng(int threadId) {
        state = WyRand::genSeed(threadId);
    }
    result_type operator()() {
        return WyRand::next_r(&state);
    }
};

/**
 * Provides STL compatible RNG over Xoroshiro128PP.
 */
class Xoroshiro128PPRng {
private:
    uint32_t state[4];
public:
    using result_type = uint32_t;
    constexpr result_type min() { return 0; }
    constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    Xoroshiro128PPRng(int threadId) {
        Xoroshiro128PP::genSeed(threadId);
        for (int i = 0; i < 4; ++i) {
            state[i] = Xoroshiro128PP::state[i];
        }
    }
    result_type operator()() {
        return Xoroshiro128PP::next_r(state);
    }
};

}

#endif