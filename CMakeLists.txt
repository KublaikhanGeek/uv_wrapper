cmake_minimum_required(VERSION 2.6)

project(control_engine  C)

if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE "Release")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(C_FLAGS
 #-Werror
 -Wall
 -Wextra
 -Wconversion
 -Wno-unused-parameter
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -ffast-math
 -rdynamic
 )

string(REPLACE ";" " " CMAKE_C_FLAGS "${C_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -fsanitize=address")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(samples)
