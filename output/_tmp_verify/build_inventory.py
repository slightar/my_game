"""Inventory built executables and compare their age against the edited sources.

If my_game.exe is older than enemy_renderer.cpp, the user is running a stale binary
and that alone explains 'the bug is still there'.
"""
import os
import time
from pathlib import Path

ROOT = Path(r"D:\my_game")
INTERESTING = ["my_game.exe", "my_game", "fix_verify.exe", "bash_trace.exe"]

print("=== built executables under the project ===")
rows = []
for dirpath, dirnames, filenames in os.walk(ROOT):
    # skip the enormous vendored raylib source trees
    low = dirpath.lower()
    if "\\third_party" in low or "\\_deps\\raylib-src" in low:
        continue
    for fn in filenames:
        if fn.lower().endswith(".exe"):
            p = Path(dirpath) / fn
            try:
                st = p.stat()
            except OSError:
                continue
            rows.append((st.st_mtime, st.st_size, p))
rows.sort(reverse=True)
for mtime, size, p in rows[:40]:
    print(f"{time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(mtime))}  {size:>12,}  {p}")

print(f"\n(total {len(rows)} exe files)")

print("\n=== source files touched by the fix ===")
for rel in ["src/enemy_renderer.cpp", "src/enemy_system.cpp", "src/ui_input.h",
            "src/terminal_ui.cpp", "src/menu_views.cpp", "src/game.cpp",
            "assets/enemies/mobs/manifest.json", "assets/enemies/mobs/shield_v2.png"]:
    p = ROOT / rel
    if p.exists():
        st = p.stat()
        print(f"{time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(st.st_mtime))}  {st.st_size:>12,}  {rel}")
    else:
        print(f"{'(missing)':>19}  {'':>12}  {rel}")

print("\n=== raylib static lib used for offline builds ===")
for cand in ROOT.glob("cmake-build-*/_deps/raylib-build/raylib/libraylib.a"):
    st = cand.stat()
    print(f"{time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(st.st_mtime))}  {st.st_size:>12,}  {cand}")
