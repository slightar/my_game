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
        src/player.cpp
        src/boss.cpp
        src/audio_system.cpp
        src/ui_font.cpp
        src/platform_input.cpp)
target_link_libraries(my_game PRIVATE raylib)

if (WIN32)
    target_link_libraries(my_game PRIVATE imm32)
endif()
