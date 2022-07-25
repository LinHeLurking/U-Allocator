project(U-Allocator)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)

add_library(Allocator STATIC allocator.h allocator.cpp)

add_executable(test_allocator_seperate tests/seperate_allocator.cpp)
target_link_libraries(test_allocator_seperate PRIVATE Allocator)

add_executable(test_allocator_interchange tests/interchange_allocator.cpp)
target_link_libraries(test_allocator_interchange PRIVATE Allocator)

enable_testing()

add_test(NAME test_allocator_seperate COMMAND $<TARGET_FILE:test_allocator_seperate>)
add_test(NAME test_allocator_interchange COMMAND $<TARGET_FILE:test_allocator_interchange>)
