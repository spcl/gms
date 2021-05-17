add_subdirectory(third_party)
add_subdirectory(algorithms/preprocessing)
add_subdirectory(algorithms/set_based)
add_subdirectory(algorithms/non_set_based)

if (BUILD_GAPBS_BENCHMARKS)
    add_subdirectory(representations/graphs/log_graph)
endif ()
