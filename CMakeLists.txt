cmake_minimum_required(VERSION 3.0...3.5)
project(memory)

set(HEADERS
  include/sp/bit_iterator.h
  include/sp/node_iterator.h
  include/sp/pointer_iterator.h
  include/sp/reverse_iterator.h
  include/sp/reserving_allocator.h
  include/sp/pool_allocator.h
)

set(TEST_SOURCES
    tests/main.cc
    tests/test_pool_allocator.cc
)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(INSTALL_GTEST OFF)

include(EnableGoogleTest)
include(CTest)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/sp)

add_executable(
    unit_tests
    ${TEST_SOURCES}
)
target_include_directories(unit_tests PRIVATE include)
target_link_libraries(
    unit_tests
    GTest::gtest_main
)
gtest_discover_tests(unit_tests)
