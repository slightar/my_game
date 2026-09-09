# 代码 1：CMakeLists.txt（节选）
cmake_minimum_required(VERSION 3.20)
project(my_game CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5          # 固定版本，保证依赖可复现
)
FetchContent_MakeAvailable(raylib)

add_executable(my_game
        src/main.cpp
        src/game.cpp
        src/character_art.cpp
        src/player.cpp
        src/boss.cpp
        src/audio_system.cpp
        src/main_menu.cpp
        src/ui_font.cpp
        src/platform_input.cpp)
target_link_libraries(my_game PRIVATE raylib)

add_custom_command(TARGET my_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:my_game>/assets)

if (WIN32)
    target_link_libraries(my_game PRIVATE imm32)
    if (MINGW)
        target_link_options(my_game PRIVATE -static-libgcc -static-libstdc++)
        get_filename_component(MINGW_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        add_custom_command(TARGET my_game POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MINGW_RUNTIME_DIR}/libwinpthread-1.dll"
                        $<TARGET_FILE_DIR:my_game>)
    endif()
endif()
