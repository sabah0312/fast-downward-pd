# See https://www.fast-downward.org/ForDevelopers/CMake
# for general information on adding source files and CMake libraries.
#
# All libraries are enabled by default and users can disable them by specifying
#    -DLIBRARY_FOO_ENABLED=FALSE
# The default behavior can be changed so all non-essential libraries are
# disabled by default by specifying
#    -DDISABLE_LIBRARIES_BY_DEFAULT=TRUE
# In that case, individual libraries can be enabled with
#    -DLIBRARY_FOO_ENABLED=TRUE
#
# Defining a new library:
#    create_fast_downward_library(
#        NAME <NAME>
#        [ HELP <HELP> ]
#        SOURCES
#            <FILE_1> [ <FILE_2> ... ]
#        [ DEPENDS <LIBRARY_NAME_1> [ <LIBRARY_NAME_2> ... ] ]
#        [ DEPENDENCY_ONLY ]
#        [ CORE_LIBRARY ]
#    )
#
# <HELP> is used to describe the cmake option, for example in ccmake.
# SOURCES lists the source files that are part of the library. Entries are
#    listed without extension. For an entry <file>, both <file>.h and <file>.cc
#    are added if the files exist.
# DEPENDS lists libraries that will be compiled as dependendies if this library
#    is enabled.
# DEPENDENCY_ONLY disables the library unless it is needed as a dependency and
#    hides the option to enable the library in cmake GUIs like ccmake.
# CORE_LIBRARY always enables the library (even if DISABLE_LIBRARIES_BY_DEFAULT
#    is used) and hides the option to disable it in CMake GUIs like ccmake.

cmake_minimum_required(VERSION 3.16)

# Path containing custom CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


report_bitwidth()
set_up_build_types("Debug;Release")
set_up_options()

project(downward LANGUAGES CXX)


# In the following, we include all source files, grouped into libraries with
# dependencies among each other.
create_fast_downward_library(
        NAME block_basics_sources
        HELP "Block, Ordering constraints, Manager source files"
        SOURCES
            myutils.h
            block
            ordering_constraints
            deorder_manager
            orderings
            block_deorder_plan
            deorder_algorithm
            deorder_algorithms/eog
            deorder_algorithms/block_deorder
            deorder_algorithms/fibs
            deorder_algorithms/cibs
            block_replace
            plan_reduction
            cluster_causal_graph
            plan_decompose
            variable_cluster
)


create_fast_downward_library(
        NAME deorder_plugin_sources
        HELP "Deorder plugin source files"
        SOURCES
        deorder_algorithms/plugin_eog.cc
        deorder_algorithms/plugin_block_deorder.cc
        deorder_algorithms/plugin_fibs.cc
        deorder_algorithms/plugin_cibs.cc
)




