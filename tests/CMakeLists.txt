cmake_minimum_required(VERSION 3.0...3.5)
project(memory)

set(CMAKE_CXX_STANDARD 14)

include_directories(../include)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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
