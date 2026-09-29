"""Offline g++ driver for the project (no cmake/ninja/make available).

Builds enemy_tests, then optionally the full game. Paths mirror the toolchain recorded
in memory: MSYS2 g++ plus the raylib artefacts already sitting in cmake-build-debug.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(r"D:\my_game")
GXX = r"C:\msys64\ucrt64\bin\g++.exe"
RL_SRC = ROOT / "cmake-build-debug/_deps/raylib-src/src"
RL_LIB = ROOT / "cmake-build-debug/_deps/raylib-build/raylib/libraylib.a"

COMMON = [
    "-std=gnu++20", "-g", "-DGRAPHICS_API_OPENGL_33", "-DPLATFORM_DESKTOP",
    f"-I{ROOT/'src'}", f"-I{ROOT/'third_party'}", f"-I{RL_SRC}",
    f"-I{RL_SRC/'external/glfw/include'}",
]
LIBS = [
    "-lopengl32", "-lglu32", "-lwinmm", "-lkernel32", "-luser32", "-lgdi32",
    "-lwinspool", "-lshell32", "-lole32", "-loleaut32", "-luuid", "-lcomdlg32",
    "-ladvapi32", "-limm32", "-static-libgcc", "-static-libstdc++",
]


def build(name, sources, exe, extra=(), with_raylib=True):
    cmd = [GXX, *COMMON, *[str(s) for s in sources], *extra]
    if with_raylib:
        cmd.append(str(RL_LIB))
    cmd += [*LIBS, "-o", str(exe)]
    print(f"--- building {name} ---", flush=True)
    result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    out = (result.stdout + result.stderr).strip()
    if out:
        print(out[-6000:])
    print(f"{name}: rc={result.returncode}")
    return result.returncode


ENEMY_CORE = [
    ROOT / "src/enemy_data.cpp",
    ROOT / "src/enemy_system.cpp",
]

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "tests"
    rc = 0

    if what in ("tests", "all"):
        # Mirrors the CMake target exactly: enemy_tests links only the simulation, so it
        # stays headless and never pulls raylib drawing symbols.
        rc |= build(
            "enemy_tests",
            [ROOT / "tests/enemy_tests.cpp", *ENEMY_CORE],
            ROOT / "cmake-build-debug/enemy_tests.exe",
        )

    if what in ("game", "all"):
        # Mirrors the my_game target in CMakeLists.txt. platform_input.cpp owns
        # PrepareGameWindowInput(), which main.cpp calls - omitting it leaves main.cpp
        # unresolved even though every other TU links.
        game_sources = [
            ROOT / "src/main.cpp", ROOT / "src/game.cpp", ROOT / "src/boss.cpp",
            ROOT / "src/enemy_data.cpp", ROOT / "src/enemy_system.cpp", ROOT / "src/enemy_renderer.cpp",
            ROOT / "src/character.cpp", ROOT / "src/character_repository.cpp",
            ROOT / "src/character_animation.cpp", ROOT / "src/character_art.cpp",
            ROOT / "src/player.cpp", ROOT / "src/audio_system.cpp", ROOT / "src/main_menu.cpp",
            ROOT / "src/menu_views.cpp", ROOT / "src/terminal_ui.cpp", ROOT / "src/story_data.cpp",
            ROOT / "src/progress_system.cpp", ROOT / "src/game_settings.cpp", ROOT / "src/ui_font.cpp",
            ROOT / "src/platform_input.cpp",
        ]
        missing = [s for s in game_sources if not s.is_file()]
        if missing:
            print("missing game sources:", missing)
        else:
            rc |= build("my_game", game_sources, ROOT / "cmake-build-debug/my_game.exe")

    if what in ("smoke", "all"):
        # Manual GPU capture harness; hidden window so it runs headless-ish. Writes into
        # character-qa/ next to the exe, which is where the captured PNGs are collected.
        smoke_sources = [
            ROOT / "tests/character_render_smoke.cpp",
            ROOT / "src/character.cpp", ROOT / "src/character_repository.cpp",
            ROOT / "src/character_animation.cpp", ROOT / "src/character_art.cpp",
            ROOT / "src/enemy_data.cpp", ROOT / "src/enemy_system.cpp", ROOT / "src/enemy_renderer.cpp",
            ROOT / "src/player.cpp", ROOT / "src/audio_system.cpp", ROOT / "src/main_menu.cpp",
            ROOT / "src/menu_views.cpp", ROOT / "src/terminal_ui.cpp", ROOT / "src/story_data.cpp",
            ROOT / "src/progress_system.cpp", ROOT / "src/game_settings.cpp", ROOT / "src/ui_font.cpp",
        ]
        rc |= build("character_render_smoke", smoke_sources,
                    ROOT / "cmake-build-debug/character_render_smoke.exe")

    if what in ("gamesmoke", "all"):
        # Integration smoke for the trial itself: entry, assist-aim gunfire, character
        # switch, pause and the return to the boss demo. Run it via
        # output/_tmp_new/run_gamesmoke.py, which copies the exe + assets into a scratch
        # directory - Game reads/writes save + settings next to the exe, so running it in
        # place would touch the real build directory's progress file.
        gamesmoke_sources = [
            ROOT / "tests/game_enemy_smoke.cpp",
            ROOT / "src/game.cpp", ROOT / "src/boss.cpp",
            ROOT / "src/character.cpp", ROOT / "src/character_repository.cpp",
            ROOT / "src/character_animation.cpp", ROOT / "src/character_art.cpp",
            ROOT / "src/enemy_data.cpp", ROOT / "src/enemy_system.cpp", ROOT / "src/enemy_renderer.cpp",
            ROOT / "src/player.cpp", ROOT / "src/audio_system.cpp", ROOT / "src/main_menu.cpp",
            ROOT / "src/menu_views.cpp", ROOT / "src/terminal_ui.cpp", ROOT / "src/story_data.cpp",
            ROOT / "src/progress_system.cpp", ROOT / "src/game_settings.cpp", ROOT / "src/ui_font.cpp",
        ]
        rc |= build("game_enemy_smoke", gamesmoke_sources,
                    ROOT / "cmake-build-debug/game_enemy_smoke.exe")

    sys.exit(rc)
