"""Locate the shield PLATE per frame (pose-independent facing cue).

Alpha centroid measures how the body leans, which for a bash animation swings
back during the windup -- so it reports "the pose leans left" for frames that are
actually authored facing right. The shield plate is a rigid object attached to one
arm; its side relative to the torso does not change with the pose, so it is a much
better facing cue.
"""
import os
from PIL import Image

BASE = r"D:\my_game\assets\enemies\mobs"
SHEETS = ["shield_v2.png", "shield.png", "soldier.png", "crossbow.png", "slug.png", "exploder.png", "drone.png"]
CELL = 192


def analyse(path):
    im = Image.open(path).convert("RGBA")
    px = im.load()
    print("=" * 78)
    print(os.path.basename(path), im.size)
    for row in range(4):
        print("  row %d" % row)
        for col in range(10):
            x0, y0 = col * CELL, row * CELL
            minx, maxx, miny, maxy = 10 ** 9, -1, 10 ** 9, -1
            sx = n = 0
            rows_alpha = []
            for y in range(y0, y0 + CELL):
                run = 0
                run_start = -1
                best_run = 0
                best_start = -1
                gap = 0
                for x in range(x0, x0 + CELL):
                    if px[x, y][3] > 40:
                        if run_start < 0:
                            run_start = x - x0
                        run += 1
                        gap = 0
                        if x - x0 < minx:
                            minx = x - x0
                        if x - x0 > maxx:
                            maxx = x - x0
                        if y - y0 < miny:
                            miny = y - y0
                        if y - y0 > maxy:
                            maxy = y - y0
                        sx += x - x0
                        n += 1
                    else:
                        # tolerate 2px pinholes inside the plate
                        if run and gap < 2:
                            gap += 1
                            run += 1
                        else:
                            if run > best_run:
                                best_run, best_start = run, run_start
                            run = 0
                            run_start = -1
                            gap = 0
                if run > best_run:
                    best_run, best_start = run, run_start
                rows_alpha.append((best_run, best_start, y - y0))
            if n == 0:
                print("    f%-2d empty" % col)
                continue
            cx = sx / n
            bcx = (minx + maxx) / 2
            # widest horizontal opaque run in the cell = the shield plate edge
            wrun, wstart, wy = max(rows_alpha)
            plate_c = wstart + wrun / 2
            print("    f%-2d lean%+7.1f  plateX%+7.1f  plateW%5d  plateY%5d  bboxW%4d"
                  % (col, cx - bcx, plate_c - bcx, wrun, wy, maxx - minx))


for s in SHEETS:
    p = os.path.join(BASE, s)
    if os.path.exists(p):
        analyse(p)
    else:
        print("missing:", p)
