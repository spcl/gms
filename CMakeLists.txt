cmake_minimum_required(VERSION 3.8)
project(gms)

option(PEDANTIC_WARNINGS "if OFF some warnings will be disabled" OFF)
option(DEBUG_WITH_SANITIZERS "whether to build debug builds with sanitizers" OFF)
option(BUILD_TESTS "whether to build tests" ON)
option(BUILD_GAPBS_BENCHMARKS "whether to build GAPBS benchmarks parameterized over compressible graphs" OFF)

# Compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fopenmp -march=native")
if (PEDANTIC_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder -Wno-sign-compare -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
    # TODO
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shift-negative-value")
    # TODO this silences warnings in Roaring library
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DMINEBENCH_TEST")
if (DEBUG_WITH_SANITIZERS)
    # - Address sanitization to detect many memory bugs at a low cost.
    # - UB sanitizer to detect some cases of UB.
    # - Note: on clang (when supported) you might also use the memory sanitizer instead of address
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined")
endif()

# Release flags:
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Collect outputs in a single bin directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable release mode by default (if nothing else is specified).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Setup TBB
#include(cmake/tbb/TBBGet.cmake)
#tbb_get(TBB_ROOT tbb_root RELEASE_TAG v2020.2 CONFIG_DIR TBB_DIR)
#find_package(TBB)

# Detect if CPLEX is available.
include(cmake/FindCPLEX.cmake)

# Setup PAPI
include(cmake/FindPAPI.cmake)
if(PAPI_FOUND)
    message (STATUS "PAPI was FOUND: Including Papi Directories")
    include_directories(${PAPI_INCLUDE_DIRS})
endif(PAPI_FOUND)

# Add a benchmark target
# @param bench_file The .cpp file containing the benchmark runner.
# @param PAPIW Optional flag to enable PAPIW Support (if not set, than false)
function(gms_benchmark bench_file)
    cmake_parse_arguments(PARSE_ARGV 1 FLAG "PAPIW" "" "")
    get_filename_component(bench_name ${bench_file} NAME_WE) # get filename without extension
    add_executable(${bench_name} ${bench_file})
    target_link_libraries(${bench_name} roaring)
    if(PAPI_FOUND AND FLAG_PAPIW)
        message (STATUS "PAPIW ENABLED for ${bench_file}" )
        target_link_libraries(${bench_name} ${PAPI_LIBRARIES})
    else()
        target_compile_definitions(${bench_name} PUBLIC NOPAPIW)
    endif()
endfunction()

include_directories(${CMAKE_CURRENT_LIST_DIR}/.)

add_subdirectory(gms)
add_subdirectory(examples)

if (BUILD_TESTS)
    include(ExternalProject)
    externalproject_add(googletest
            SOURCE_DIR        "${CMAKE_CURRENT_LIST_DIR}/testing/googletest"
            PREFIX            "${CMAKE_BINARY_DIR}/googletest"
            INSTALL_COMMAND   "")
    set(GOOGLETEST_INCLUDE_DIRS
            ${CMAKE_CURRENT_LIST_DIR}/testing/googletest/googletest/include
            ${CMAKE_CURRENT_LIST_DIR}/testing/googletest/googlemock/include
            )
    link_directories(${CMAKE_BINARY_DIR}/googletest/src/googletest-build/lib)
    set(TESTING_FLAGS -DTEST_FIXTURES="${CMAKE_CURRENT_LIST_DIR}/testing" -DMINEBENCH_TEST)
    add_subdirectory(testing)
endif()
