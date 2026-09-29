"""Did the on-disk build actually recompile the fixed sources?

The exe timestamp alone is misleading: a link can happen without recompiling.  The
per-source object files are the ground truth.  If enemy_renderer.cpp.obj is older than
enemy_renderer.cpp, the running binary still contains the pre-fix renderer and the user
is testing stale code no matter what the exe's date says.
"""
import os
import time
from pathlib import Path

ROOT = Path(r"D:\my_game")
SOURCES = ["enemy_renderer.cpp", "enemy_system.cpp", "game.cpp", "terminal_ui.cpp", "menu_views.cpp"]

print("=== object files (compiled translation units) ===")
objs = []
for dirpath, _dirnames, filenames in os.walk(ROOT):
    low = dirpath.lower()
    if "\\third_party" in low or "\\_deps\\raylib-src" in low:
        continue
    for fn in filenames:
        if fn.endswith(".obj") or fn.lower().endswith(".o"):
            p = Path(dirpath) / fn
            try:
                st = p.stat()
            except OSError:
                continue
            objs.append((st.st_mtime, st.st_size, p))

for name in SOURCES:
    hits = [(m, s, p) for m, s, p in objs if p.name == name + ".obj" or p.name == name + ".o"]
    src = ROOT / "src" / name
    src_m = src.stat().st_mtime if src.exists() else 0
    print(f"\n--- {name}  (source {time.strftime('%m-%d %H:%M:%S', time.localtime(src_m))}) ---")
    if not hits:
        print("    no object file found")
    for m, s, p in sorted(hits, reverse=True)[:6]:
        verdict = "OK (compiled after source edit)" if m >= src_m else "STALE (older than source)"
        print(f"    {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(m))}  {s:>10,}  {p}")
        print(f"        -> {verdict}")

print("\n=== most recently compiled objects overall (top 15) ===")
for m, s, p in sorted(objs, reverse=True)[:15]:
    print(f"  {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(m))}  {p}")
