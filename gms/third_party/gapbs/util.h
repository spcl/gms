// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "timer.h"


/*
GAP Benchmark Suite
Author: Scott Beamer

Miscellaneous helpers that don't fit into classes
*/


static const int64_t kRandSeed = 27491095;

void PrintLabel(const std::string &label, const std::string &val) {
    printf("%-21s%7s\n", (label + ":").c_str(), val.c_str());
}

void PrintTime(const std::string &s, double seconds) {
    printf("%-21s%3.5lf\n", (s + ":").c_str(), seconds);
}

void PrintStep(const std::string &s, int64_t count) {
    printf("%-14s%14" PRId64 "\n", (s + ":").c_str(), count);
}

void PrintStep(int step, double seconds, int64_t count) {
    if (count != -1)
        printf("%5d%11" PRId64 "  %10.5lf\n", step, count, seconds);
    else
        printf("%5d%23.5lf\n", step, seconds);
}

void PrintStep(const std::string &s, double seconds, int64_t count) {
    if (count != -1)
        printf("%5s%11" PRId64 "  %10.5lf\n", s.c_str(), count, seconds);
    else
        printf("%5s%23.5lf\n", s.c_str(), seconds);
}

// Runs op and prints the time it took to execute labelled by label
#define TIME_PRINT(label, op) {   \
  Timer t_;                       \
  t_.Start();                     \
  (op);                           \
  t_.Stop();                      \
  PrintTime(label, t_.Seconds()); \
}


template <typename T_>
class RangeIter {
  T_ x_;
 public:
  explicit RangeIter(T_ x) : x_(x) {}
  bool operator!=(RangeIter const& other) const { return x_ != other.x_; }
  T_ const& operator*() const { return x_; }
  RangeIter& operator++() {
    ++x_;
    return *this;
  }
};

template <typename T_>
class Range{
  T_ from_;
  T_ to_;
 public:
  explicit Range(T_ to) : from_(0), to_(to) {}
  Range(T_ from, T_ to) : from_(from), to_(to) {}
  RangeIter<T_> begin() const { return RangeIter<T_>(from_); }
  RangeIter<T_> end() const { return RangeIter<T_>(to_); }
};

template <typename T>
std::string vec2str(const std::vector<T> vec, const std::string& sep = ", ") {
    std::ostringstream result;
    for (auto el : vec) {
      result << el << sep;
    }    
    return result.str();
}

// added by Yannick Schaffner on 30.12.2019
template<class print_T>
void PrintBenchmarkOutput(print_T arg)
{
  try
  {
    std::cout << arg << std::endl;
  }
  catch(const std::exception& e)
  {
    std::cout << "NaN" << std::endl;
  }
}
// added by Yannick Schaffner on 30.12.2019
template<class print_T, class ... rprint_T>
void PrintBenchmarkOutput(print_T arg, rprint_T ... args)
{
    try 
    {
      std::cout << arg << " ";
    }
    catch (...)
    {
      std::cout << "NaN" << " ";
    }
    PrintBenchmarkOutput(args...);
}

// added by Yannick Schaffner on 30.12.2019
// template<class ... print_T>
// void PrintBenchmarkOutput(print_T ... args)
// {
//   std::cout << "@@@";
//   PrintBenchmarkOutput(std::cout, args...);
// }

template<bool...> struct string_check {};
template <> struct string_check<>: std::true_type {};
template<bool...b> struct string_check<false, b...>: std::false_type {};
template<bool...b> struct string_check<true, b...>: string_check<b...>{};


#endif  // UTIL_H_
