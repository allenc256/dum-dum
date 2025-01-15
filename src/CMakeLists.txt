include(FetchContent)

set(ABSL_PROPAGATE_CXX_STD ON)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v3.1
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    absl
    GIT_REPOSITORY  https://github.com/abseil/abseil-cpp.git
    GIT_TAG         20240722.0
)
FetchContent_MakeAvailable(absl)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CXX_FLAGS /W4 /WX)
else()
  set(CXX_FLAGS -Wall -Wextra -Werror -Wno-gcc-compat -Wno-sign-conversion -march=native)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CXX_FLAGS ${CXX_FLAGS} -Wpedantic -Wconversion)
  endif()
endif()

set(LINK_LIBS absl::flat_hash_map)

add_executable(dumdum main.cpp ${SOURCES})
add_library(dumdum_test_lib STATIC ${SOURCES})

target_compile_options(dumdum PRIVATE ${CXX_FLAGS})
target_compile_options(dumdum_test_lib PRIVATE ${CXX_FLAGS})

target_include_directories(dumdum_test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dumdum ${LINK_LIBS} argparse)
target_link_libraries(dumdum_test_lib ${LINK_LIBS})
