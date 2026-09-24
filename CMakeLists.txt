cmake_minimum_required(VERSION 3.16)

# project name
project(CachePerformanceScript LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    execute_process(
        COMMAND sysctl -n hw.cachelinesize
        OUTPUT_VARIABLE LINE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    execute_process(
        COMMAND getconf LEVEL1_DCACHE_LINESIZE
        OUTPUT_VARIABLE LINE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
if(NOT LINE OR LINE EQUAL 0)
    # We set cacheline size to 64 if none of the branches execute
    set(LINE 64)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The benchmark sources minus main
add_library(
    cache_bench_lib
    STATIC
    src/system_info/system_info.cpp
    src/cache_algorithms/experiments.cpp
    src/report/report.cpp
)

# add to the header search path
target_include_directories(cache_bench_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(cache_bench_lib PUBLIC CACHE_LINE=${LINE})
target_compile_options(cache_bench_lib PUBLIC -O2 -g -Wall -Wextra)

# The executable target for the benchmark.
add_executable(cache_bench src/main.cpp)
target_link_libraries(cache_bench PRIVATE cache_bench_lib)

# GTEST
option(CACHE_BENCH_BUILD_TESTS "Build the GoogleTest suite" ON)
if(CACHE_BENCH_BUILD_TESTS)
    enable_testing()
    set(CMAKE_CTEST_ARGUMENTS "-LE;slow")
    add_subdirectory(test)
endif()
