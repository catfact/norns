cmake_minimum_required(VERSION 3.19)
project(writer)

set(CMAKE_CXX_STANDARD 14)

add_executable(writer main.cpp)

target_link_libraries(writer rt pthread)