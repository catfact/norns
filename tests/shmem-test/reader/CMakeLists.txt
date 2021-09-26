cmake_minimum_required(VERSION 3.19)
project(shm-reader C)

set(CMAKE_C_STANDARD 11)

add_executable(shm-reader main.c)

target_link_libraries(shm-reader pthread rt)