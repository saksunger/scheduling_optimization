cmake_minimum_required(VERSION 3.5)
project(scheduling)

add_subdirectory(lib)

add_executable(scheduling main.cpp)
add_executable(performance_test performance_test.cpp)

target_link_libraries(scheduling PRIVATE commonlib)
target_link_libraries(performance_test PRIVATE commonlib)