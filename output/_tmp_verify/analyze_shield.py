"""Independent check of the shield sheet's per-frame native facing.

Row 0 = Idle, row 1 = Move_Loop, row 2 = Attack, row 3 = Die.
For each cell we report how the alpha mass is split left/right of the cell centre
(192 px cells, centre x = 96).  The shield soldier holds a large buckler on the
side it faces, so a positive 'bias' means the pose is drawn facing right.
"""
import sys
from PIL import Image

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
CELL = 192

img = Image.open(SHEET).convert("RGBA")
print(f"sheet size = {img.size} (expect 1920x768)")

for row in range(4):
    print(f"\n=== row {row} ===")
    print("frame  coverage  centroid-96  left%  right%  bias")
    for frame in range(10):
        cell = img.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
        alpha = cell.getchannel("A")
        px = list(alpha.getdata())
        total = 0
        weighted = 0
        left = 0
        right = 0
        for y in range(CELL):
            base = y * CELL
            for x in range(CELL):
                a = px[base + x]
                if a < 24:
                    continue
                total += a
                weighted += x * a
                if x < CELL // 2:
                    left += a
                else:
                    right += a
        if total == 0:
            print(f"{frame:5d}  (blank)")
            continue
        centroid = weighted / total
        lp = 100.0 * left / total
        rp = 100.0 * right / total
        cov = 0
        for a in px:
            if a >= 24:
                cov += 1
        print(f"{frame:5d}  {cov:8d}  {centroid - CELL/2:+11.2f}  {lp:5.1f}  {rp:5.1f}  "
              f"{'RIGHT' if centroid > CELL/2 else 'LEFT '}")
