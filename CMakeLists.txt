cmake_minimum_required(VERSION 3.14)

project(dum-dum)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(ENABLE_COVERAGE true)
else()
  set(ENABLE_COVERAGE false)
endif()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(abseil-cpp)
add_subdirectory(PicoSHA2)

if (ENABLE_COVERAGE)
  add_custom_target(clean_coverage
    COMMAND rm -rf "${CMAKE_BINARY_DIR}/**/*.gcda"
  )
  add_dependencies(dumdum clean_coverage)
  add_dependencies(dumdum_test clean_coverage)
endif()
