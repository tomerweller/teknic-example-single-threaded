cmake_minimum_required(VERSION 3.6)
project(teknic_example_single_threaded)

set(CMAKE_CXX_STANDARD 11)

include_directories(foundation)

set(SOURCE_FILES main.cpp Axis.h Axis.cpp)

add_executable(teknic_example_single_threaded ${SOURCE_FILES})