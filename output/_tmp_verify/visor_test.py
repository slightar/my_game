"""Objective visor test.

The helmet carries a bright reflective visor plate.  Its horizontal position relative to
the rest of the head is a facing cue that does not depend on the whole silhouette.

Calibration rows (manifest: row0 Idle native LEFT, row1 Walk native RIGHT):
  the visor offset should have OPPOSITE signs for those two rows.
Then we read the attack row natively: whichever of the two calibration signs its frames
match tells us which way each frame was authored.
"""
from pathlib import Path
from PIL import Image

SHEET = Image.open(r"D:\my_game\assets\enemies\mobs\shield_v2.png").convert("RGBA")
CELL = 192
HEAD_TOP, HEAD_BOT = 10, 80
CENTRE = CELL / 2.0


def visor_offset(row, frame):
    cell = SHEET.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
    px = cell.load()
    pts = []
    for y in range(HEAD_TOP, HEAD_BOT):
        for x in range(CELL):
            r, g, b, a = px[x, y]
            if a < 200:
                continue
            lum = 0.299 * r + 0.587 * g + 0.114 * b
            pts.append((x, y, lum, a))
    if not pts:
        return None
    # whole-head centroid (alpha weighted)
    tot = sum(p[3] for p in pts)
    head_c = sum(p[0] * p[3] for p in pts) / tot
    # visor = brightest 6% of opaque head pixels
    pts.sort(key=lambda p: -p[2])
    top = pts[:max(1, len(pts) * 6 // 100)]
    visor_c = sum(p[0] for p in top) / len(top)
    visor_lum = sum(p[2] for p in top) / len(top)
    return visor_c - head_c, visor_lum, len(top)


print("calibration (manifest: row0 native LEFT, row1 native RIGHT)")
for row, name in ((0, "Idle"), (1, "Walk")):
    vals = [visor_offset(row, f)[0] for f in range(10)]
    avg = sum(vals) / len(vals)
    print(f"  r{row} {name:<5} visor offsets: " +
          " ".join(f"{v:+5.1f}" for v in vals) + f"   avg {avg:+5.2f}")

print("\nattack row (row 2), native sheet")
vals = []
for f in range(10):
    off, lum, n = visor_offset(2, f)
    vals.append(off)
    print(f"  f{f}  visor offset {off:+6.2f}   (visor pixels {n}, mean luma {lum:.0f})")

print("\nrow 3 (Die) native")
print("  " + " ".join(f"{visor_offset(3, f)[0]:+5.1f}" for f in range(10)))
