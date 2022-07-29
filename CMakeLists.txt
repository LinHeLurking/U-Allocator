project(U-Allocator)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_library(
    Allocator 
    STATIC 
    src/allocator.cpp
)

add_executable(test_allocator_seperate tests/seperate_allocator.cpp)
target_link_libraries(test_allocator_seperate PUBLIC Allocator)
target_link_libraries(test_allocator_seperate PUBLIC Threads::Threads)

add_executable(test_allocator_interchange tests/interchange_allocator.cpp)
target_link_libraries(test_allocator_interchange PUBLIC Allocator)
target_link_libraries(test_allocator_interchange PUBLIC Threads::Threads)

add_executable(perf_allocator tests/perf_allocator.cpp)
target_link_libraries(perf_allocator PUBLIC Allocator)
target_link_libraries(perf_allocator PUBLIC Threads::Threads)

add_executable(test_radix_tree tests/test_radix_tree.cpp)
add_executable(perf_radix_tree tests/perf_radix_tree.cpp)

enable_testing()

add_test(NAME test_allocator_seperate COMMAND $<TARGET_FILE:test_allocator_seperate>)
add_test(NAME test_allocator_interchange COMMAND $<TARGET_FILE:test_allocator_interchange>)
add_test(NAME test_radix_tree COMMAND $<TARGET_FILE:test_radix_tree>)
