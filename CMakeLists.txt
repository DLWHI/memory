cmake_minimum_required(VERSION 3.0...3.5)
project(memory LANGUAGES C CXX)
message(STATUS ${CMAKE_SYSTEM_VERSION})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
  include/allocators/pool_allocator.h
  include/containers/array.h
  include/containers/vector.h
  # include/sp/list.h
  include/iterators/bit_iterator.h
  include/iterators/node_iterator.h
  include/iterators/pointer_iterator.h
  include/iterators/reverse_iterator.h
  include/iterators/reserving_allocator.h
)

set(TEST_SOURCES
    tests/main.cc
    tests/allocators/test_pool_allocator.cc
    tests/containers/test_array.cc
    tests/containers/test_vector.cc
)

set(CMAKE_MODULE_PATH 
    ${CMAKE_SOURCE_DIR}/cmake
)

set(INSTALL_GTEST OFF)

include(EnableGoogleTest)
include(SetPlatformFlags)
include(CTest)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/sp)


if (GTest_FOUND)
  add_executable(
      unit_tests
      tests/main.cc
      ${TEST_SOURCES}
  )
  target_include_directories(unit_tests PRIVATE include)
  target_link_libraries(
      unit_tests
      GTest::gtest_main
  )
  gtest_discover_tests(unit_tests)
endif()
