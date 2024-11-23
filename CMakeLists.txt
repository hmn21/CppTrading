cmake_minimum_required(VERSION 3.22)

set (CMAKE_CXX_STANDARD 23)

project(CppTrading)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
else()
  set(CMAKE_CXX_FLAGS "-march=x86-64-v3 -flto")
endif()

add_subdirectory(lib)
add_subdirectory(app)
