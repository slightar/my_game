"""Isolate the biggest opaque blob in each attack frame (the buckler) and report which
side of the cell it sits on, plus the overall silhouette bounding box.

The shield is the largest single object the soldier carries, and a guard holds it on
the side it faces, so the blob's position is a strong, pose-independent facing cue --
stronger than a whole-cell centroid that mixes in the body and both arms.
"""
import sys
from collections import deque
from PIL import Image

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
CELL = 192
CENTRE = CELL / 2.0

sys.setrecursionlimit(1000000)
img = Image.open(SHEET).convert("RGBA")


def analyse(row, frame, mirror=False):
    cell = img.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
    if mirror:
        cell = cell.transpose(Image.FLIP_LEFT_RIGHT)
    a = cell.getchannel("A")
    px = a.load()
    solid = [[px[x, y] >= 24 for x in range(CELL)] for y in range(CELL)]

    seen = [[False] * CELL for _ in range(CELL)]
    blobs = []
    for sy in range(CELL):
        for sx in range(CELL):
            if not solid[sy][sx] or seen[sy][sx]:
                continue
            q = deque([(sx, sy)])
            seen[sy][sx] = True
            n = 0
            wx = wy = 0
            x0, x1, y0, y1 = sx, sx, sy, sy
            while q:
                x, y = q.popleft()
                n += 1
                wx += x
                wy += y
                x0 = min(x0, x)
                x1 = max(x1, x)
                y0 = min(y0, y)
                y1 = max(y1, y)
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < CELL and 0 <= ny < CELL and solid[ny][nx] and not seen[ny][nx]:
                        seen[ny][nx] = True
                        q.append((nx, ny))
            blobs.append((n, wx / n - CENTRE, wy / n, (x0, y0, x1, y1)))
    blobs.sort(reverse=True)
    return blobs


def line(row, frame, label, mirror=False):
    blobs = analyse(row, frame, mirror)
    if not blobs:
        print(f"  {label:22s} blank")
        return
    n, cx, cy, bb = blobs[0]
    total = sum(b[0] for b in blobs)
    side = "RIGHT" if cx > 0 else "LEFT "
    print(f"  {label:22s} largest blob {n:6d}px ({n/total*100:4.1f}% of ink) "
          f"centroid x-C={cx:+7.2f} ({side})  y-C={cy - CENTRE:+6.1f}  bbox={bb}")


print("=== references ===")
line(0, 0, "idle f0 (LEFT)")
line(1, 0, "walk f0 (RIGHT)")
line(1, 0, "walk f0 mirrored", mirror=True)

print("\n=== attack row 2, native ===")
for f in range(10):
    line(2, f, f"row2 f{f} native")

print("\n=== attack row 2, bash mirrored (frames 4-8) ===")
for f in range(4, 10):
    line(2, f, f"row2 f{f} mirrored", mirror=True)

print("\nThe buckler's centroid side, computed on the native sheet, tells us which way")
print("each frame was drawn.  Frames sharing the wind-up's side were authored facing the")
print("same way as the idle clip; the rest need mirroring for a left-facing guard.")
