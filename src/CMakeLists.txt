file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

if (MSVC)
  set(CXX_FLAGS /W4 /WX)
else()
  set(CXX_FLAGS -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-gcc-compat -Wno-sign-conversion)
endif()

set(LINK_LIBS absl::flat_hash_map picosha2)

add_executable(dumdum main.cpp ${SOURCES})
add_library(dumdum_test_lib STATIC ${SOURCES})

target_compile_options(dumdum PRIVATE ${CXX_FLAGS})
target_compile_options(dumdum_test_lib PRIVATE ${CXX_FLAGS} --coverage)

target_include_directories(dumdum_test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dumdum ${LINK_LIBS})
target_link_libraries(dumdum_test_lib ${LINK_LIBS})

file(GLOB_RECURSE COVERAGE_FILES ${CMAKE_BINARY_DIR}/**/*.gcno)
list(TRANSFORM COVERAGE_FILES REPLACE "[.]gcno$" ".gcda")
add_custom_target(clean_coverage_src
  COMMAND ${CMAKE_COMMAND} -E remove -f "${COVERAGE_FILES}"
  COMMAND_EXPAND_LISTS
)
add_dependencies(dumdum_test_lib clean_coverage_src)

