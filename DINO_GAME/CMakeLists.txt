cmake_minimum_required(VERSION 3.14)
project(dino_game C)

set(CMAKE_C_STANDARD 99)

include(FetchContent)

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
)
FetchContent_MakeAvailable(lvgl)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(dino_game main.c)

target_include_directories(dino_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(dino_game PRIVATE
    lvgl
    ${SDL2_LIBRARIES}
    m
    pthread
)

target_compile_options(dino_game PRIVATE ${SDL2_CFLAGS_OTHER})
