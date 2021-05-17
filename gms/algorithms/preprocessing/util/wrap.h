#pragma once
#include <gms/common/types.h>

template <class AnyGraph, class Fn>
constexpr auto preprocessing_wrap_return(Fn fn) {
    return [fn](const AnyGraph &g) {
        std::vector<NodeId> ordering;
        fn(g, ordering);
        return ordering;
    };
}

template <class AnyGraph, class Fn>
constexpr auto preprocessing_wrap_return(Fn fn, double epsilon) {
    return [fn, epsilon](const AnyGraph &g) {
        std::vector<NodeId> ordering;
        fn(g, ordering, epsilon);
        return ordering;
    };
}

template <class Fn>
constexpr auto preprocessing_bind(Fn fn, double epsilon) {
    return [fn, epsilon](const auto &g, auto &output) {
        fn(g, output, epsilon);
    };
}