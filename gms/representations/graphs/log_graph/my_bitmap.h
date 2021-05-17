// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef MY_BITMAP_H_
#define MY_BITMAP_H_

#include <algorithm>
#include <cinttypes>

#include <gms/third_party/gapbs/platform_atomics.h>


/*
GAP Benchmark Suite
Class:  Bitmap
Author: Scott Beamer,

Parallel bitmap that is thread-safe
 - Can set bits in parallel (set_bit_atomic) unlike std::vector<bool>
*/


class My_Bitmap {
 public:
  explicit My_Bitmap(size_t size) {
    uint64_t num_words = (size + 64- 1) / 64;
    // start_ = new uint64_t[num_words];
    start_ = (uint64_t*) calloc(num_words, sizeof(int64_t));
	this->size = size;
  }

  ~My_Bitmap() {
    // delete[] start_;
	free(start_);
  }

  void set_bit(size_t pos) {
    start_[word_offset(pos)] |= ((uint64_t) 1l << bit_offset(pos));
  }

  void set_bit_atomic(size_t pos) {
    uint64_t old_val, new_val;
    do {
      old_val = start_[word_offset(pos)];
      new_val = old_val | ((uint64_t) 1l << bit_offset(pos));
    } while (!compare_and_swap(start_[word_offset(pos)], old_val, new_val));
  }

  bool get_bit(size_t pos) const {
    // return (start_[word_offset(pos)] >> bit_offset(pos)) & 1l;
    return (start_[pos >> 6] >> (pos & 63)) & 1l;
  }

  uint8_t get_2bits(size_t pos) const {
    // return (start_[word_offset(pos)] >> bit_offset(pos)) & 3l;
    return (start_[pos >> 6] >> (pos & 63)) & 3l;
  }

  int64_t get_size(){
	  return size;
  }

 private:
  uint64_t *start_;
  int64_t size;

  static uint64_t word_offset(size_t n) { return n / 64; }
  static uint64_t bit_offset(size_t n) { return n & (64- 1); }
};

#endif  // MY_BITMAP_H_
