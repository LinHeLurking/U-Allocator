project(U-Allocator)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(test_allocator_seperate tests/seperate_allocator.cpp)
target_link_libraries(test_allocator_seperate PUBLIC Threads::Threads)

add_executable(test_allocator_interchange tests/interchange_allocator.cpp)
target_link_libraries(test_allocator_interchange PUBLIC Threads::Threads)

add_executable(perf_allocator tests/perf_allocator.cpp)
target_link_libraries(perf_allocator PUBLIC Threads::Threads)

add_executable(test_mem_pool tests/test_mem_pool.cpp)
target_link_libraries(test_mem_pool PUBLIC Threads::Threads)

enable_testing()

add_test(NAME test_mem_pool COMMAND $<TARGET_FILE:test_mem_pool>)
