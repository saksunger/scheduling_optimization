cmake_minimum_required(VERSION 3.5)
project(scheduling)

add_subdirectory(lib)

add_executable(scheduling main.cpp)

target_link_libraries(scheduling PRIVATE commonlib)
