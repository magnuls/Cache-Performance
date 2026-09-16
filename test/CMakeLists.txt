find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unit_tests
	sattolo_test.cpp
	chase_test.cpp
	node_config_test.cpp
	report_test.cpp
	system_info_test.cpp
)
target_link_libraries(unit_tests PRIVATE cache_bench_lib GTest::gtest_main)
gtest_discover_tests(unit_tests)

# Full sweeps take minutes; run with `ctest -L slow`. Registered as one test so
# the read sweep shared by the fixture runs once.
add_executable(sweep_tests sweep_test.cpp)
target_link_libraries(sweep_tests PRIVATE cache_bench_lib GTest::gtest_main)
add_test(NAME sweep_tests COMMAND sweep_tests)
set_tests_properties(sweep_tests PROPERTIES LABELS slow)
