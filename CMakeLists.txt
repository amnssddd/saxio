cmake_minimum_required(VERSION 3.31)
project(saxio)

set(CMAKE_CXX_STANDARD 23)

add_compile_definitions(NEED_SAXIO_LOG)

find_package(TBB REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(DEPEND_LIBS
        tbb
)

add_subdirectory(tests)
