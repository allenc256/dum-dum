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

include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp)

add_executable(dumdum_test ${TEST_SOURCES})
target_link_libraries(dumdum_test dumdum_test_lib GTest::gmock GTest::gtest_main)

gtest_discover_tests(dumdum_test)

add_executable(dumdum_benchmark benchmark.cpp)
target_link_libraries(dumdum_benchmark dumdum_test_lib benchmark::benchmark)

