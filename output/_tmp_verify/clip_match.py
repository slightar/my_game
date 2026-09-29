"""Do the attack frames match 'Idle' or the 'Move' clip? This is the decisive check.

Rows 2/3 of every sheet are the Attack/Die clips, which by construction drive the
character FORWARD (the same direction as the walk), so they are authored like
Move_Loop. If a cell's silhouette is identical to an idle cell it is authored
facing the idle way; if it matches the walk it is authored facing the walk way.
Comparing cells is self-consistent and needs no assumption about which way is
"left" or "right", so it cannot be fooled by a front-on pose.
"""
import os
from PIL import Image

BASE = r"D:\my_game\assets\enemies\mobs"
CELL = 192


def cell(sheet, row, col):
    im = Image.open(os.path.join(BASE, sheet)).convert("RGBA")
    return im.crop((col * CELL, row * CELL, col * CELL + CELL, row * CELL + CELL))


def dist(a, b):
    """Mean per-pixel alpha difference; 0 = identical silhouettes."""
    pa, pb = a.getchannel("A").load(), b.getchannel("A").load()
    s = 0
    for y in range(CELL):
        for x in range(CELL):
            s += abs(pa[x, y] - pb[x, y])
    return s / (CELL * CELL)


def signed_bias(img):
    """Alpha-weighted centroid x minus bounding-box centre (the 'lean')."""
    a = img.getchannel("A").load()
    sx = n = 0
    minx, maxx = CELL, -1
    for y in range(CELL):
        for x in range(CELL):
            if a[x, y] > 60:
                sx += x
                n += 1
                minx = min(minx, x)
                maxx = max(maxx, x)
    if n == 0:
        return 0.0
    return sx / n - (minx + maxx) / 2


for sheet in ("shield_v2.png", "soldier.png", "crossbow.png", "exploder.png"):
    print("=" * 74)
    print(sheet)
    idle = [cell(sheet, 0, c) for c in range(10)]
    move = [cell(sheet, 1, c) for c in range(10)]
    print("  clip self-consistency (mean alpha diff within each row):")
    for name, row in (("idle", idle), ("move", move)):
        ds = [dist(row[i], row[(i + 1) % 10]) for i in range(9)]
        print("    %s  neighbour diff avg %.1f  (small = the row keeps one direction)"
              % (name, sum(ds) / len(ds)))
    print("  per-cell lean (centroid - bbox centre):")
    for name, row in (("idle", idle), ("move", move)):
        print("    %s  %s" % (name, " ".join("%+5.1f" % signed_bias(c) for c in row)))
    for row_i, row_name in ((2, "attack"), (3, "die")):
        cells = [cell(sheet, row_i, c) for c in range(10)]
        print("  %s row: lean = %s" % (row_name, " ".join("%+5.1f" % signed_bias(c) for c in cells)))
        print("    nearest idle frame -> distance | nearest move frame -> distance")
        for c in range(10):
            di = min(range(10), key=lambda k: dist(cells[c], idle[k]))
            dm = min(range(10), key=lambda k: dist(cells[c], move[k]))
            vi, vm = dist(cells[c], idle[di]), dist(cells[c], move[dm])
            tag = "IDLE-like" if vi < vm else "MOVE-like"
            print("      f%-2d  idle f%-2d %.1f | move f%-2d %.1f  -> %s" % (c, di, vi, dm, vm, tag))
