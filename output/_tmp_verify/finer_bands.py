"""Narrow-band profile of the shield's attack row.

If the bash frames were authored as "body facing LEFT but the buckler swung out to the
RIGHT", then the whole-cell centroid would sit right (dominated by the big shield) while
the head and feet would still sit left.  That case would mean mirroring the frame turns
the BODY the wrong way.

So print, per frame, the centroid of six 32px-tall bands plus each band's x-extent.
Head band = y 0..32, feet band = y 160..192.
"""
from pathlib import Path
from PIL import Image

SHEET = Path(r"D:\my_game\assets\enemies\mobs\shield_v2.png")
CELL = 192
CENTRE = CELL / 2.0
BANDS = [(0, 32), (32, 64), (64, 96), (96, 128), (128, 160), (160, 192)]
BAND_NAMES = ["0-32 ", "32-64", "64-96", "96-128", "128-160", "160-192"]

img = Image.open(SHEET).convert("RGBA")


def band_stats(cell, y0, y1):
    a = cell.getchannel("A")
    px = a.load()
    tot = w = 0
    lo, hi = None, None
    for y in range(y0, y1):
        for x in range(CELL):
            v = px[x, y]
            if v < 24:
                continue
            tot += v
            w += x * v
            lo = x if lo is None else min(lo, x)
            hi = x if hi is None else max(hi, x)
    if not tot:
        return None, None, None
    return w / tot - CENTRE, lo - CENTRE, hi - CENTRE


print("attack row (row 2) of shield_v2.png")
print("each cell: centroid | leftmost | rightmost   (all relative to cell centre 96)\n")
header = "frame |" + "".join(f"{n:>22}|" for n in BAND_NAMES)
print(header)
for f in range(10):
    cell = img.crop((f * CELL, 2 * CELL, f * CELL + CELL, 3 * CELL))
    parts = []
    for y0, y1 in BANDS:
        c, lo, hi = band_stats(cell, y0, y1)
        if c is None:
            parts.append(f"{'--':>22}")
        else:
            parts.append(f"{c:+6.1f} {lo:+6.0f} {hi:+6.0f}   ")
    print(f"{f:5d} |" + "|".join(parts) + "|")

print("\nRead it as: if the head band and the feet band agree in sign, the whole body is")
print("drawn that way.  If they disagree, the pose is twisted and the centroid is not a")
print("facing cue.")
