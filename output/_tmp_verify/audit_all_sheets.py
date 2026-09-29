"""Audit EVERY frame of EVERY enemy sheet for internal facing consistency.

The renderer mirrors a cell based on a per-ROW native-facing value, so a row is only
safe if all ten of its frames were authored facing the same way.  A single stray frame
will visibly flip that one pose.  This sweeps all rows of all sheets and reports any
row that mixes directions, using two independent cues:

  legs  - alpha centroid of the lower band (y 128..186), the feet step toward facing
  all   - alpha centroid of the whole cell
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(r"D:\my_game\assets\enemies\mobs")
CELL = 192
CENTRE = CELL / 2.0
LEG_BAND = (128, 186)

ROWS = ["Idle", "Move_Loop", "Attack", "Die"]


def bias(cell_img, y0=0, y1=CELL):
    a = cell_img.getchannel("A")
    px = a.load()
    tot = 0
    w = 0
    for y in range(y0, y1):
        for x in range(CELL):
            v = px[x, y]
            if v < 24:
                continue
            tot += v
            w += x * v
    return None if tot == 0 else w / tot - CENTRE


def sign(v, dead=4.0):
    if v is None:
        return "?"
    if abs(v) < dead:
        return "."          # too close to centre to call
    return "R" if v > 0 else "L"


sheets = sorted(p for p in ROOT.glob("*.png") if not p.name.startswith("shield-fix"))
for sheet in sheets:
    img = Image.open(sheet).convert("RGBA")
    if img.size != (1920, 768):
        print(f"\n{sheet.name}: unexpected size {img.size}, skipped")
        continue
    print(f"\n=== {sheet.name} ===")
    for r in range(4):
        leg = []
        whole = []
        for f in range(10):
            cell = img.crop((f * CELL, r * CELL, f * CELL + CELL, r * CELL + CELL))
            lb = bias(cell, *LEG_BAND)
            wb = bias(cell)
            leg.append(sign(lb))
            whole.append(sign(wb))
        legstr = "".join(leg)
        wholestr = "".join(whole)
        faces = {c for c in legstr if c in "LR"}
        flag = "  <-- MIXED" if len(faces) > 1 else ""
        print(f"  row {r} {ROWS[r]:<10} legs {legstr}   all {wholestr}{flag}")

print("\nL=left R=right . =too central to call.  A row containing both L and R was")
print("authored with mixed directions and needs per-frame handling.")
