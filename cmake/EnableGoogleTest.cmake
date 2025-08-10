if (POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

find_package(GTest CONFIG)

if(NOT GTest_FOUND AND FETCH_GTEST)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    EXCLUDE_FROM_ALL
  )

if (WIN32)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif (WIN32)
  FetchContent_MakeAvailable(googletest)
endif()

if(GTest_FOUND)
  enable_testing()

  include(GoogleTest)
endif()
