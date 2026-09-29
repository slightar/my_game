"""Check which way each newly baked rig natively faces.

The baker hard-codes facing [1,1,1,1] (0 "native right") for everything except the
shield, and the shield proved that guess can be wrong. Rather than trust it, print a
density map of the Idle and mid-Attack cell so the head / weapon side is readable.

Reference: 士兵 and 弩手 are independently known to be native-right, so they are printed
alongside as a control.
"""
import sys
from pathlib import Path

from PIL import Image

MOBS = Path(r"D:\my_game\assets\enemies\mobs")
TARGETS = [
    ("soldier", "soldier.png", "CONTROL native-right"),
    ("crossbow", "crossbow.png", "CONTROL native-right"),
    ("hound", "hound.png", "enemy_1000_gopro"),
    ("acid_slug", "acid_slug.png", "enemy_1004_mslime"),
    ("caster", "caster.png", "enemy_1011_wizard"),
    ("stealth_crossbow", "stealth_crossbow.png", "enemy_1019_jshoot"),
    ("slug_high", "slug_high.png", "enemy_1021_bslime  (was exploder)"),
]

COLS, ROWS = 46, 22


def show(path, row, frame, label):
    im = Image.open(path).getchannel("A")
    cell = im.crop((frame * 192, row * 192, frame * 192 + 192, row * 192 + 192))
    bbox = cell.getbbox()
    if not bbox:
        print(f"    [{label}] cell empty")
        return
    x0, y0, x1, y1 = bbox
    px = cell.load()
    print(f"    [{label}] ink bbox x {x0}-{x1} (w {x1-x0}), y {y0}-{y1} (h {y1-y0})")
    for r in range(ROWS):
        line = ""
        for c in range(COLS):
            xa = x0 + (x1 - x0) * c // COLS
            xb = max(xa + 1, x0 + (x1 - x0) * (c + 1) // COLS)
            ya = y0 + (y1 - y0) * r // ROWS
            yb = max(ya + 1, y0 + (y1 - y0) * (r + 1) // ROWS)
            tot = 0
            n = 0
            for y in range(ya, yb):
                for x in range(xa, xb):
                    tot += px[x, y]
                    n += 1
            v = tot / max(n, 1)
            line += " .:-=+*#%@"[min(9, int(v / 25.6))]
        print("      " + line)


for key, fname, note in TARGETS:
    path = MOBS / fname
    print("=" * 78)
    print(f"{key}   {note}")
    show(path, 0, 0, "Idle f0")
    show(path, 2, 5, "Attack f5")
