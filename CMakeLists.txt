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

add_library(character_core src/character.cpp src/character_repository.cpp src/character_animation.cpp)
target_include_directories(character_core PUBLIC src third_party)
target_link_libraries(character_core PUBLIC raylib)

add_executable(character_tool src/character_tool.cpp)
target_link_libraries(character_tool PRIVATE character_core)

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
target_link_libraries(my_game PRIVATE character_core)

include(CTest)
if (BUILD_TESTING)
    add_executable(character_tests tests/character_tests.cpp)
    target_link_libraries(character_tests PRIVATE character_core)
    add_test(NAME character_tests COMMAND character_tests)
    add_executable(character_render_smoke tests/character_render_smoke.cpp
        src/character_art.cpp src/player.cpp src/audio_system.cpp src/main_menu.cpp src/ui_font.cpp)
    target_link_libraries(character_render_smoke PRIVATE character_core)
endif()

add_custom_command(TARGET my_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:my_game>/assets)

if (WIN32)
    target_link_libraries(my_game PRIVATE imm32)
    if (MINGW)
        set(GAME_EXECUTABLES my_game character_tool)
        if (BUILD_TESTING)
            list(APPEND GAME_EXECUTABLES character_tests character_render_smoke)
        endif()
        get_filename_component(MINGW_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        foreach(GAME_EXECUTABLE IN LISTS GAME_EXECUTABLES)
            target_link_options(${GAME_EXECUTABLE} PRIVATE -static-libgcc -static-libstdc++)
            add_custom_command(TARGET ${GAME_EXECUTABLE} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MINGW_RUNTIME_DIR}/libwinpthread-1.dll"
                        $<TARGET_FILE_DIR:${GAME_EXECUTABLE}>)
        endforeach()
        target_link_options(character_tool PRIVATE -municode)
    endif()
endif()
