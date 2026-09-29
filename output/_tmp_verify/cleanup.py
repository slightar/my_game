"""Remove only the scratch files created while verifying the two bug fixes.

Everything here lives in gitignored directories (output/, cmake-build-debug/), so
this is pure tidiness. The verification source, its log and the render evidence
are kept on purpose.
"""
import os
import shutil

BASE = r"D:\my_game"

REMOVE_FILES = [
    r"output\check_terminal_ui.txt",
    r"output\check_menu_views.txt",
    r"output\check_enemy_renderer.txt",
    r"output\check_game.txt",
    r"output\verify_build.txt",
    r"output\probe2_build.txt",
    r"output\probe2_run.txt",
    r"output\_tmp_verify\input_probe2.cpp",
    r"cmake-build-debug\input_probe2.exe",
    r"cmake-build-debug\shield_probe.exe",
    r"cmake-build-debug\shield-probe.png",
    # Left over from the earlier analysis pass.
    r"output\_tmp_enemy_rows\input_probe.cpp",
]

REMOVE_DIRS = [
    r"cmake-build-debug\fix-verify-fixture",
    r"output\_tmp_smoke",
]

KEEP = [
    r"output\_tmp_verify\fix_verify.cpp",
    r"output\_tmp_verify\verify_run.txt",
    r"output\_tmp_verify\analyze_shield.py",
    r"output\_tmp_verify\zoom_bash.py",
    r"output\_tmp_verify\shield-bash-zoom.png",
    r"cmake-build-debug\fix_verify.exe",
    r"cmake-build-debug\shield-fix-sheet.png",
]

for name in REMOVE_FILES:
    path = os.path.join(BASE, name)
    if os.path.isfile(path):
        os.remove(path)
        print(f"removed file {name}")
    else:
        print(f"absent       {name}")

for name in REMOVE_DIRS:
    path = os.path.join(BASE, name)
    if os.path.isdir(path):
        shutil.rmtree(path)
        print(f"removed dir  {name}")
    else:
        print(f"absent       {name}")

print("\nkept (verification evidence):")
for name in KEEP:
    path = os.path.join(BASE, name)
    print(f"  {'ok    ' if os.path.exists(path) else 'MISSING'} {name}")
