file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" *h)

add_executable(dumdum Main.cpp ${SOURCES})
add_library(dumdum_lib STATIC ${SOURCES})
target_include_directories(dumdum_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(dumdum PRIVATE /W4 /WX)
else()
  target_compile_options(dumdum PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion)
endif()
