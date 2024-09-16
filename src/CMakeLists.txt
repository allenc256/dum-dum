file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_executable(dumdum main.cpp ${SOURCES})
add_library(dumdum_test_lib STATIC ${SOURCES})
target_include_directories(dumdum_test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(dumdum PRIVATE /W4 /WX)
else()
  target_compile_options(dumdum PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion)
  target_compile_options(dumdum_test_lib PRIVATE --coverage)
endif()

file(GLOB_RECURSE COVERAGE_FILES ${CMAKE_BINARY_DIR}/**/*.gcno)
list(TRANSFORM COVERAGE_FILES REPLACE "[.]gcno$" ".gcda")
add_custom_target(clean_coverage_src
  COMMAND ${CMAKE_COMMAND} -E remove -f "${COVERAGE_FILES}"
  COMMAND_EXPAND_LISTS
)
add_dependencies(dumdum_test_lib clean_coverage_src)

