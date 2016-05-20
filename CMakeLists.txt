cmake_minimum_required(VERSION 3.3)
project(ex4OS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp CachingFileSystem.cpp)
add_executable(ex4OS ${SOURCE_FILES})