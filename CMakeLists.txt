cmake_minimum_required(VERSION 3.5)

project(a LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # using Clang
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
endif()

find_package(Boost REQUIRED)

add_executable(
  ${PROJECT_NAME}
  main.cpp
)
