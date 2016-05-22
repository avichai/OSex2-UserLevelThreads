cmake_minimum_required(VERSION 3.3)
project(ex4Final)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES FBR.h FBR.cpp CachingFileSystem.cpp)
add_executable(ex4Final ${SOURCE_FILES})