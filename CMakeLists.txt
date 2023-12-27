cmake_minimum_required(VERSION 3.0...3.5)
project(memory)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(INSTALL_GTEST OFF)

set(HEADERS
  include/sp/memory.h
  include/sp/node_iterator.h
  include/sp/pointer_iterator.h
  include/sp/reverse_iterator.h
  include/sp/reserving_allocator.h
  include/sp/pool_allocator.h
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/sp)

include(EnableGoogleTest)

add_executable(
  unit_tests
  tests/test_pool_allocator.cc
  tests/test_reserving_allocator.cc
)
target_link_libraries(
  unit_tests
  GTest::gtest_main
)
gtest_discover_tests(unit_tests)
