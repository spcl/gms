# This module finds cplex.
#
# Based on: https://github.com/martinsch/pgmlink/blob/master/cmake_extensions/FindCplex.cmake
#
# User can give CPLEX_ROOT_DIR as a hint stored in the cmake cache.
#
# It sets the following variables:
#  CPLEX_FOUND              - Set to false, or undefined, if cplex isn't found.
#  CPLEX_INCLUDE_DIRS       - include directory
#  CPLEX_LIBRARIES          - library files

set(CPLEX_ROOT_DIR "$ENV{CPLEX_DIR}" CACHE PATH "Root directory of CPLEX if it's not in a standard place")

FIND_PATH(CPLEX_INCLUDE_DIR
        ilcplex/cplex.h
        HINTS
        ${CPLEX_ROOT_DIR}/cplex/include
        ${CPLEX_ROOT_DIR}/include
        PATHS
        ENV C_INCLUDE_PATH
        ENV C_PLUS_INCLUDE_PATH
        ENV INCLUDE_PATH
        )

FIND_PATH(CPLEX_CONCERT_INCLUDE_DIR
        ilconcert/iloenv.h
        HINTS
        ${CPLEX_ROOT_DIR}/concert/include
        ${CPLEX_ROOT_DIR}/include
        PATHS
        ENV C_INCLUDE_PATH
        ENV C_PLUS_INCLUDE_PATH
        ENV INCLUDE_PATH
        )

FIND_LIBRARY(CPLEX_LIBRARY
        NAMES cplex
        HINTS
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_osx/static_pic
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_darwin/static_pic
        PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
        )

FIND_LIBRARY(CPLEX_ILOCPLEX_LIBRARY
        ilocplex
        HINTS
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_osx/static_pic
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_darwin/static_pic
        PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
        )

FIND_LIBRARY(CPLEX_CONCERT_LIBRARY
        concert
        HINTS
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_linux/static_pic
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_osx/static_pic
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_darwin/static_pic
        PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
        )

FIND_PATH(CPLEX_BIN_DIR
        cplex
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux
        ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_osx
        ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_darwin
        ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
        )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CPLEX DEFAULT_MSG
        CPLEX_LIBRARY CPLEX_INCLUDE_DIR CPLEX_ILOCPLEX_LIBRARY CPLEX_CONCERT_LIBRARY CPLEX_CONCERT_INCLUDE_DIR)

if(CPLEX_FOUND)
    SET(CPLEX_INCLUDE_DIRS ${CPLEX_INCLUDE_DIR} ${CPLEX_CONCERT_INCLUDE_DIR})
    SET(CPLEX_LIBRARIES ${CPLEX_CONCERT_LIBRARY} ${CPLEX_ILOCPLEX_LIBRARY} ${CPLEX_LIBRARY} )
    SET(CPLEX_LIBRARIES "${CPLEX_LIBRARIES};m;pthread")
endif()

MARK_AS_ADVANCED(CPLEX_LIBRARY CPLEX_INCLUDE_DIR CPLEX_ILOCPLEX_LIBRARY CPLEX_CONCERT_INCLUDE_DIR CPLEX_CONCERT_LIBRARY)
