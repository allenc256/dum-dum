file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

if (MSVC)
  set(CXX_FLAGS /W4 /WX)
else()
  set(CXX_FLAGS -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-gcc-compat -Wno-sign-conversion)
endif()

if (ENABLE_COVERAGE)
  set(CXX_COVERAGE_FLAGS --coverage)
endif()

set(LINK_LIBS absl::flat_hash_map picosha2)

add_executable(dumdum main.cpp ${SOURCES})
add_library(dumdum_test_lib STATIC ${SOURCES})

target_compile_options(dumdum PRIVATE ${CXX_FLAGS})
target_compile_options(dumdum_test_lib PRIVATE ${CXX_FLAGS} ${CXX_COVERAGE_FLAGS})

target_include_directories(dumdum_test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dumdum ${LINK_LIBS})
target_link_libraries(dumdum_test_lib ${LINK_LIBS})

if (ENABLE_COVERAGE)
  add_custom_command(
    TARGET dumdum_test_lib
    PRE_BUILD
    COMMAND rm -rf "${CMAKE_BINARY_DIR}/src/**/*.gcda"
  )
endif()
