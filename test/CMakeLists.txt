include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(CMAKE_CXX_FLAGS --coverage)

include(GoogleTest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")

add_executable(dumdum_test ${SOURCES})
target_link_libraries(dumdum_test dumdum_test_lib GTest::gmock GTest::gtest_main)

gtest_discover_tests(dumdum_test)

# Required to avoid code coverage errors on linking.
file(GLOB_RECURSE COVERAGE_FILES ${CMAKE_BINARY_DIR}/**/*.gcda)
if(NOT "${COVERAGE_FILES}" STREQUAL "")
  file(REMOVE_RECURSE ${COVERAGE_FILES})
endif()
