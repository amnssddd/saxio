cmake_minimum_required(VERSION 3.31)
project(saxio)

set(CMAKE_CXX_STANDARD 23)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(saxio tests/main.cpp)
