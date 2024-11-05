cmake_minimum_required(VERSION 3.14)

project(dum-dum)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(abseil-cpp)
add_subdirectory(PicoSHA2)
add_subdirectory(benchmark)
