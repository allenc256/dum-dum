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

add_executable(dumdum_test Cards_test.cpp Game_test.cpp)
target_link_libraries(dumdum_test dumdum_lib GTest::gmock GTest::gtest_main)

gtest_discover_tests(dumdum_test)
