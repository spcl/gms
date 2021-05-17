set(test_sources
        bron_kerbosch.cpp
        clique_counting.cpp
        preprocessing/preprocessing.cpp
        sets.cpp
        cgraph.cpp
        coders.cpp
        set_graph.cpp
        )

foreach(source_file ${test_sources})
    get_filename_component(name ${source_file} NAME_WE)
    add_executable(${name}_test ${source_file})
    target_link_libraries(${name}_test roaring gtest gtest_main gmock)
    target_compile_definitions(${name}_test PUBLIC NOPAPIW)
    target_compile_definitions(${name}_test PUBLIC ${TESTING_FLAGS})
    target_include_directories(${name}_test PUBLIC ${GOOGLETEST_INCLUDE_DIRS})
    add_dependencies(${name}_test googletest)
    # NOTE: OUTPUT_NAME removes the redundant _test suffix again, which is added to avoid clashes with the main benchmark binaries.
    set_target_properties(${name}_test
            PROPERTIES
            OUTPUT_NAME ${name}
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endforeach()
