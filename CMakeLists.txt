cmake_minimum_required(VERSION 3.0...3.5)
project(containers)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(_DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)

include_directories(include)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(EnableGoogleTest)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# valgrind  --tool=memcheck --track-fds=yes --trace-children=yes --track-origins=yes --leak-check=full --show-leak-kinds=all -s --log-file=leak_report.txt ./test_vector
  add_compile_options(-Wall -Werror -Wextra -Wimplicit-fallthrough -Wpedantic -g)
  # add_link_options(-fsanitize=address)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
  add_compile_options(-Wall -Werror -Wextra -Wimplicit-fallthrough -Wpedantic -O3)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4 /EHsc /fsanitize=address)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  add_compile_options(-Wall -Werror -Wextra -Wimplicit-fallthrough -Wpedantic -O3)
  # add_link_options(-fsanitize=address)
endif()


add_executable(
  unit_tests
  tests/test_pool_allocator.cc
)
target_link_libraries(
    unit_tests
  GTest::gtest_main
)
gtest_discover_tests(unit_tests)