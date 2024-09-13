set(SOURCES Main.cpp Cards.cpp Cards.h State.cpp State.h Panic.h)

add_executable(dumdum ${SOURCES})
add_library(dumdum_lib STATIC ${SOURCES})
target_include_directories(dumdum_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(dumdum PRIVATE /W4 /WX)
else()
  target_compile_options(dumdum PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion)
endif()
