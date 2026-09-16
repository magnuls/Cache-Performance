cmake_minimum_required(VERSION 3.16)

# Project name and the language it uses.
project(CachePerformanceScript LANGUAGES CXX)

# Use C++17 and make it a hard requirement 
# Disable compiler-specific extensions for portability.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
	execute_process(COMMAND sysctl -n hw.cachelinesize
		OUTPUT_VARIABLE LINE OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
	execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE
                  OUTPUT_VARIABLE LINE OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT LINE OR LINE EQUAL 0)
# We set cacheline size to 64 if none of the branches execute
  set(LINE 64)
endif()

# No build type on purpose. Release and RelWithDebInfo both define NDEBUG,
# which compiles out every assert() in the benchmark; Debug leaves the code
# at -O0, which makes cache measurements meaningless. Leaving CMAKE_BUILD_TYPE
# empty means CMake contributes no flags at all, so the optimisation level
# below is the only one in play and NDEBUG is never defined.

# Export compile_commands.json so clangd (your editor's linter/LSP)
# can find headers and flags automatically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The benchmark sources minus main, built once and shared by cache_bench and the tests.
add_library(cache_bench_lib STATIC
	src/system_info/system_info.cpp
	src/cache_algorithms/experiments.cpp
	src/report/report.cpp
)

# add to the header search path
target_include_directories(cache_bench_lib PUBLIC ${CMAKE_SOURCE_DIR}/src )

# Warnings help catch mistakes early. These flags are for GCC/Clang
# (AppleClang on macOS included). -O2 -g here rather than via a build type,
# see the CMAKE_BUILD_TYPE note above.
target_compile_definitions(cache_bench_lib PUBLIC CACHE_LINE=${LINE})
target_compile_options(cache_bench_lib PUBLIC -O2 -g -Wall -Wextra)

# The executable target for the benchmark.
add_executable(cache_bench src/main.cpp)
target_link_libraries(cache_bench PRIVATE cache_bench_lib)
