#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <gms/common/cli/cli.h>
#include <gms/third_party/gapbs/benchmark.h>
#include <gms/third_party/gapbs/reader.h>


CSRGraph loadGraphFromFile(const std::string &path, bool symmetrize) {
    using namespace GMS::CLI;

    std::string pathStr(path);
    std::string prefixStr(TEST_FIXTURES);
    std::string fullPath = prefixStr + std::string("/testGraphs/") + pathStr;

    Args args;
    args.graph_spec.is_generator = false;
    args.graph_spec.name = fullPath;
    args.symmetrize = symmetrize;

    CSRGraph g = args.load_graph();

    assert(g.directed() == !symmetrize);

    return g;
}

CSRGraph loadGraphFromFile(char const *path) {
    return loadGraphFromFile(path, true);
}