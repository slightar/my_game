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
add_library(prts_narration src/prts_narrator.cpp)
target_include_directories(prts_narration PUBLIC src)
add_library(story_bosses src/boss.cpp src/boss_renderer.cpp src/story_boss.cpp src/winter_boss.cpp src/story_boss_renderer.cpp src/winter_boss_renderer.cpp src/boss_client_fx.cpp src/crown_client_fx.cpp src/boss_audio.cpp)
target_include_directories(story_bosses PUBLIC src third_party)
target_link_libraries(story_bosses PUBLIC raylib)

add_library(developer_sandbox src/developer_room.cpp src/developer_room_view.cpp)
target_include_directories(developer_sandbox PUBLIC src)
target_link_libraries(developer_sandbox PUBLIC story_bosses)

add_library(character_core src/character.cpp src/character_repository.cpp src/character_animation.cpp)
target_include_directories(character_core PUBLIC src third_party)
target_link_libraries(character_core PUBLIC raylib)

add_executable(character_tool src/character_tool.cpp)
target_link_libraries(character_tool PRIVATE character_core)

add_executable(my_game
        src/main.cpp
        src/game.cpp src/developer_room_game.cpp src/world_preview.cpp src/world_scenery.cpp
        src/character_art.cpp
        src/player.cpp
        src/enemy_data.cpp
        src/enemy_system.cpp
        src/enemy_renderer.cpp
        src/audio_system.cpp
        src/game_settings.cpp
        src/main_menu.cpp
        src/menu_views.cpp src/world_map.cpp src/action_map.cpp src/city_stage.cpp
        src/terminal_ui.cpp
        src/story_data.cpp
        src/progress_system.cpp
        src/ui_font.cpp
        src/platform_input.cpp)
target_link_libraries(my_game PRIVATE character_core prts_narration story_bosses developer_sandbox)

include(CTest)
if (BUILD_TESTING)
    add_executable(prts_narrator_tests tests/prts_narrator_tests.cpp)
    target_link_libraries(prts_narrator_tests PRIVATE prts_narration)
    add_test(NAME prts_narrator_tests COMMAND prts_narrator_tests)
    add_executable(world_layout_tests tests/world_layout_tests.cpp)
    target_include_directories(world_layout_tests PRIVATE src)
    add_test(NAME world_layout_tests COMMAND world_layout_tests)
    add_executable(world_map_smoke tests/world_map_smoke.cpp
        src/game.cpp src/developer_room_game.cpp src/world_preview.cpp src/world_scenery.cpp src/enemy_data.cpp src/enemy_system.cpp src/enemy_renderer.cpp
        src/character_art.cpp src/player.cpp src/audio_system.cpp src/main_menu.cpp
        src/menu_views.cpp src/world_map.cpp src/action_map.cpp src/city_stage.cpp src/terminal_ui.cpp src/story_data.cpp src/progress_system.cpp src/game_settings.cpp src/ui_font.cpp)
    target_link_libraries(world_map_smoke PRIVATE character_core)
    add_executable(character_tests tests/character_tests.cpp)
    target_link_libraries(character_tests PRIVATE character_core)
    add_test(NAME character_tests COMMAND character_tests)
    add_executable(progress_tests tests/progress_tests.cpp src/progress_system.cpp src/story_data.cpp)
    target_include_directories(progress_tests PRIVATE src third_party)
    add_test(NAME progress_tests COMMAND progress_tests)
    add_executable(settings_tests tests/settings_tests.cpp src/game_settings.cpp)
    target_include_directories(settings_tests PRIVATE src third_party)
    target_link_libraries(settings_tests PRIVATE raylib)
    add_test(NAME settings_tests COMMAND settings_tests)
    add_executable(ui_interaction_tests tests/ui_interaction_tests.cpp src/story_data.cpp)
    target_include_directories(ui_interaction_tests PRIVATE src)
    target_link_libraries(ui_interaction_tests PRIVATE raylib)
    add_test(NAME ui_interaction_tests COMMAND ui_interaction_tests)
    add_executable(enemy_tests tests/enemy_tests.cpp src/enemy_data.cpp src/enemy_system.cpp)
    target_include_directories(enemy_tests PRIVATE src)
    target_link_libraries(enemy_tests PRIVATE raylib)
    add_test(NAME enemy_tests COMMAND enemy_tests)
    add_executable(chapter_roster_tests tests/chapter_roster_tests.cpp src/enemy_data.cpp)
    target_include_directories(chapter_roster_tests PRIVATE src third_party)
    add_test(NAME chapter_roster_tests COMMAND chapter_roster_tests "${CMAKE_SOURCE_DIR}/assets/enemies/mobs")
    add_executable(story_boss_tests tests/story_boss_tests.cpp)
    target_link_libraries(story_boss_tests PRIVATE story_bosses)
    add_test(NAME story_boss_tests COMMAND story_boss_tests "${CMAKE_SOURCE_DIR}/assets/enemies/bosses")
    add_executable(adaptive_difficulty_tests tests/adaptive_difficulty_tests.cpp src/enemy_data.cpp src/enemy_system.cpp)
    target_link_libraries(adaptive_difficulty_tests PRIVATE story_bosses)
    add_test(NAME adaptive_difficulty_tests COMMAND adaptive_difficulty_tests)
    add_executable(developer_room_tests tests/developer_room_tests.cpp src/enemy_data.cpp src/enemy_system.cpp)
    target_link_libraries(developer_room_tests PRIVATE developer_sandbox)
    add_test(NAME developer_room_tests COMMAND developer_room_tests)
    add_executable(winter_boss_tests tests/winter_boss_tests.cpp)
    target_link_libraries(winter_boss_tests PRIVATE story_bosses)
    add_test(NAME winter_boss_tests COMMAND winter_boss_tests "${CMAKE_SOURCE_DIR}/assets/enemies/bosses")
    add_executable(w_combat_tests tests/w_combat_tests.cpp)
    target_link_libraries(w_combat_tests PRIVATE story_bosses)
    add_test(NAME w_combat_tests COMMAND w_combat_tests)
    add_executable(boss_variety_tests tests/boss_variety_tests.cpp)
    target_link_libraries(boss_variety_tests PRIVATE story_bosses)
    add_test(NAME boss_variety_tests COMMAND boss_variety_tests)
    add_executable(character_render_smoke tests/character_render_smoke.cpp
        src/enemy_data.cpp src/enemy_system.cpp src/enemy_renderer.cpp src/character_art.cpp src/player.cpp src/audio_system.cpp src/main_menu.cpp src/menu_views.cpp src/world_map.cpp src/action_map.cpp src/city_stage.cpp src/terminal_ui.cpp src/story_data.cpp src/progress_system.cpp src/game_settings.cpp src/ui_font.cpp)
    target_link_libraries(character_render_smoke PRIVATE character_core)
    add_executable(game_enemy_smoke tests/game_enemy_smoke.cpp
        src/game.cpp src/developer_room_game.cpp src/world_preview.cpp src/world_scenery.cpp src/enemy_data.cpp src/enemy_system.cpp src/enemy_renderer.cpp
        src/character_art.cpp src/player.cpp src/audio_system.cpp src/main_menu.cpp
        src/menu_views.cpp src/world_map.cpp src/action_map.cpp src/city_stage.cpp src/terminal_ui.cpp src/story_data.cpp src/progress_system.cpp src/game_settings.cpp src/ui_font.cpp)
    target_link_libraries(game_enemy_smoke PRIVATE character_core)
    foreach(NARRATION_TARGET world_map_smoke character_render_smoke game_enemy_smoke)
        target_link_libraries(${NARRATION_TARGET} PRIVATE prts_narration story_bosses developer_sandbox)
    endforeach()
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
            list(APPEND GAME_EXECUTABLES prts_narrator_tests world_layout_tests world_map_smoke character_tests character_render_smoke progress_tests settings_tests enemy_tests chapter_roster_tests story_boss_tests adaptive_difficulty_tests w_combat_tests boss_variety_tests winter_boss_tests developer_room_tests game_enemy_smoke ui_interaction_tests)
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
