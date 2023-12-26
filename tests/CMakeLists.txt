cmake_minimum_required(VERSION 3.0...3.5)
project(memory)

set(CMAKE_CXX_STANDARD 14)

include(EnableGoogleTest)

add_executable(
  unit_tests
  test_pool_allocator.cc
  test_reserving_allocator.cc
)
target_link_libraries(
  unit_tests
  GTest::gtest_main
)
gtest_discover_tests(unit_tests)
