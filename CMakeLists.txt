cmake_minimum_required(VERSION 3.0...3.5)
project(memory)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

set(HEADERS
  include/sp/memory.h
  include/sp/node_iterator.h
  include/sp/pointer_iterator.h
  include/sp/reverse_iterator.h
  include/sp/reserving_allocator.h
  include/sp/pool_allocator.h
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/sp)
