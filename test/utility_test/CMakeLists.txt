cmake_minimum_required(VERSION 3.12)
project(utility_test)
include_directories(".")
file(GLOB SOURCES "*.cpp")
set(EXECUTABLE_NAME utility_test)
add_executable(${EXECUTABLE_NAME}
	${SOURCES}
	utility/al/Crc32.cpp)
set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 11)
add_compile_options(${EXECUTABLE_NAME} PUBLIC "-ggdb")
