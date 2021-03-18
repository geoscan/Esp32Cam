#######################################################################
# The Embedded Template Library (https://www.etlcpp.com/)
#######################################################################
cmake_minimum_required(VERSION 3.5.0)
project(libetl)

option(NO_STL "No STL" OFF)

add_library(libetl INTERFACE)

target_include_directories(libetl SYSTEM INTERFACE include)

target_compile_definitions(libetl INTERFACE)

