"""Decide the TRUE native facing of every shield attack frame from image content alone.

Why the legs: the buckler is a huge blob on the side the soldier faces, so a whole
cell centroid is dominated by the shield and can be misleading. The legs and feet,
however, always step toward the direction of travel / facing, and no weapon covers
them. So we measure the alpha centroid of a low band (the legs) separately, and
compare against the Idle row (row 0), whose facing we know from play testing.

Bands (inside a 192 px cell, anchor / feet at y = 180):
  head  y  16.. 72   -> helmet, leans toward facing
  torso y  72..128
  legs  y 128..186   -> feet, most reliable

We print, per frame, the horizontal offset of the band centroid from the cell
centre (96). Negative = mass sits left of centre.
"""
from PIL import Image

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
CELL = 192
CX = CELL / 2.0
BANDS = (("head", 16, 72), ("torso", 72, 128), ("legs", 128, 186))

img = Image.open(SHEET).convert("RGBA")
print(f"sheet = {img.size}  cell = {CELL}  centre = {CX}")


def band_centroid(row, frame, y0, y1):
    cell = img.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
    a = cell.getchannel("A")
    px = a.load()
    total = 0
    weighted = 0
    for y in range(y0, y1):
        for x in range(CELL):
            v = px[x, y]
            if v < 24:
                continue
            total += v
            weighted += x * v
    if total == 0:
        return None, 0
    return weighted / total, total


def report(row, label, frames):
    print(f"\n=== row {row}  ({label}) ===")
    header = "frame |" + "".join(f"  {b[0]:>17} |" for b in BANDS)
    print(header)
    for f in frames:
        cells = []
        for name, y0, y1 in BANDS:
            c, m = band_centroid(row, f, y0, y1)
            if c is None:
                cells.append(f"  {'(blank)':>17}")
            else:
                side = "R" if c > CX else "L"
                cells.append(f"  {c - CX:+8.2f} ({side})   ")
        print(f"{f:5d} |" + "|".join(cells) + "|")


# Reference rows: row 0 idle (known LEFT facing from gameplay).
report(0, "Idle  row (reference: known facing LEFT)", range(4))
report(1, "Walk  row (reference)", range(4))
report(2, "Attack row (the one under suspicion)", range(10))

print("\nInterpretation: the legs band is the tell. If an attack frame's legs")
print("centroid sits on the opposite side of the idle legs centroid, that frame")
print("is natively drawn facing the other way.")
