cmake_minimum_required(VERSION 3.16)

# Project name and the language it uses.
project(CachePerformanceScript LANGUAGES CXX)

# Use C++17 and make it a hard requirement 
# Disable compiler-specific extensions for portability.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Release build if the user didn't specify one.
# IMPORTANT for benchmarking: an unoptimized (Debug) build will completely
# distort cache-performance measurements. Always benchmark with -O2/-O3.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Export compile_commands.json so clangd (your editor's linter/LSP)
# can find headers and flags automatically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The executable target for the benchmark.
add_executable(cache_bench src/main.cpp
	src/system_info/system_info.cpp
	src/cache_algorithms/experiments.cpp
)

# add to the header search path
target_include_directories(cache_bench PUBLIC ${CMAKE_SOURCE_DIR}/src )

# Warnings help catch mistakes early. These flags are for GCC/Clang
# (AppleClang on macOS included).
target_compile_options(cache_bench PRIVATE -Wall -Wextra)
