cmake_minimum_required(VERSION 3.10)
project(tetris_240x320 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# Platform independent game core: no libc, no allocation, no floating point.
add_library(tetris_core STATIC
    src/tetris.c
    src/gfx.c
    src/font5x7.c
)
target_include_directories(tetris_core PUBLIC include)

# Optional automatic player: test harness and the simulator's attract mode.
# A minimal firmware port does not need it.
add_library(tetris_bot STATIC src/bot.c)
target_link_libraries(tetris_bot PUBLIC tetris_core)

# Headless harness: plays the game with the bot, checks invariants, writes PNGs.
add_executable(tetris_shots
    tools/shots.c
    tools/png_write.c
)
target_link_libraries(tetris_shots PRIVATE tetris_bot)
target_include_directories(tetris_shots PRIVATE tools)

# Desktop simulator, built only when SDL2 is available.
option(TETRIS_BUILD_SDL "Build the SDL2 simulator" ON)
if(TETRIS_BUILD_SDL)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SDL2 QUIET sdl2)
    endif()

    if(SDL2_FOUND)
        add_executable(tetris_sdl port/main_sdl.c)
        target_link_libraries(tetris_sdl PRIVATE tetris_bot ${SDL2_LIBRARIES})
        target_include_directories(tetris_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
        target_compile_options(tetris_sdl PRIVATE ${SDL2_CFLAGS_OTHER})
        target_link_directories(tetris_sdl PRIVATE ${SDL2_LIBRARY_DIRS})
    else()
        message(STATUS "SDL2 not found - skipping the simulator (install libsdl2-dev)")
    endif()
endif()

enable_testing()
add_test(NAME headless COMMAND tetris_shots ${CMAKE_CURRENT_BINARY_DIR}/shots)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shots)
