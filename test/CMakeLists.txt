find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unit_tests
	sattolo_test.cpp
	chase_test.cpp
)
target_link_libraries(unit_tests PRIVATE cache_bench_lib GTest::gtest_main)
gtest_discover_tests(unit_tests)
