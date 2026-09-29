"""Resolve the shield's authored direction using its OWN walk clip as the anchor.

The Move_Loop clip is unambiguous: the guard steps away from the viewer with the
shield held to its leading side. Attack/Die clips drive forward, so they must be
authored the same way as Move_Loop. Any attack cell whose silhouette matches a Move
cell is therefore committed to the walk direction, and cells matching Idle f0 keep
whatever direction Idle f0 has.

Here we only need one axis: for the walk cell and for the two candidate idle cells,
which horizontal half carries the bulk of the mass. That tells us whether the idle
guard faces the same way as the walk or the opposite way.
"""
import os
from PIL import Image

BASE = r"D:\my_game\assets\enemies\mobs"
CELL = 192


def stats(sheet, row, col):
    im = Image.open(os.path.join(BASE, sheet)).convert("RGBA")
    a = im.crop((col * CELL, row * CELL, col * CELL + CELL, row * CELL + CELL)).getchannel("A").load()
    px = [0] * CELL
    for y in range(CELL):
        for x in range(CELL):
            if a[x, y] > 60:
                px[x] += 1
    n = sum(px)
    cx = sum(i * px[i] for i in range(CELL)) / n
    left = sum(px[:96])
    right = sum(px[96:])
    colsum = [sum(px[i:i + 16]) for i in range(0, CELL, 16)]
    peak = max(range(len(colsum)), key=lambda k: colsum[k])
    return cx, left / n * 100, right / n * 100, peak * 16


print("cell                centroid   left%%  right%%  mass peak at x")
for label, row, col in (
    ("idle f0", 0, 0),
    ("idle f9", 0, 9),
    ("walk f0", 1, 0),
    ("walk f4", 1, 4),
    ("atk  f0", 2, 0),
    ("atk  f2", 2, 2),
    ("atk  f3", 2, 3),
    ("atk  f4", 2, 4),
    ("atk  f5", 2, 5),
    ("atk  f6", 2, 6),
    ("atk  f7", 2, 7),
    ("atk  f8", 2, 8),
):
    cx, l, r, pk = stats("shield_v2.png", row, col)
    print("%-8s row%d f%d   %6.1f   %5.1f  %5.1f   %3d" % (label, row, col, cx, l, r, pk))

print()
print("Reference - units whose direction is NOT in doubt (declared [1,1,1,1] and")
print("their attack clearly extends the weapon to the right, per the ASCII render):")
for sheet in ("soldier.png", "crossbow.png"):
    for label, row, col in (("idle f0", 0, 0), ("move f0", 1, 0), ("atk  f5", 2, 5)):
        cx, l, r, pk = stats(sheet, row, col)
        print("%-10s %-8s row%d f%d   %6.1f   %5.1f  %5.1f   %3d" % (sheet, label, row, col, cx, l, r, pk))
