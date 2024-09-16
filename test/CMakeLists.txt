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

# Required to avoid code coverage errors on linking.
file(GLOB_RECURSE COVERAGE_FILES ${CMAKE_BINARY_DIR}/**/*.gcno)
list(TRANSFORM COVERAGE_FILES REPLACE "[.]gcno$" ".gcda")
add_custom_target(clean_coverage_test
  COMMAND ${CMAKE_COMMAND} -E remove -f "${COVERAGE_FILES}"
  COMMAND_EXPAND_LISTS
)
add_dependencies(dumdum_test clean_coverage_test)

gtest_discover_tests(dumdum_test)
