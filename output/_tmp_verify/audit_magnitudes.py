"""Numeric magnitudes behind the per-row audit, so a real flip can be told apart from a
near-symmetric pose or a toppling death frame.

Also compares each row's measured native side against the side the manifest/code says
that row is authored at.  A disagreement means the renderer mirrors that row the wrong
way round for every unit of that kind.
"""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(r"D:\my_game\assets\enemies\mobs")
CELL = 192
CENTRE = CELL / 2.0
LEG = (128, 186)
ROWS = ["Idle", "Move", "Attack", "Die"]

manifest = json.loads((ROOT / "manifest.json").read_text(encoding="utf-8"))


def bias(cell, y0=0, y1=CELL):
    a = cell.getchannel("A")
    px = a.load()
    tot = w = 0
    for y in range(y0, y1):
        for x in range(CELL):
            v = px[x, y]
            if v < 24:
                continue
            tot += v
            w += x * v
    return None if tot == 0 else w / tot - CENTRE


for sheet in sorted(ROOT.glob("*.png")):
    if sheet.name.startswith("shield-fix"):
        continue
    key = sheet.stem
    # which file does the manifest point this kind at?
    declared_file = None
    declared_facing = None
    for kind, entry in manifest.items():
        if entry.get("file", kind + ".png") == sheet.name:
            declared_file = kind
            declared_facing = entry.get("facing", [1, 1, 1, 1])
    img = Image.open(sheet).convert("RGBA")
    if img.size != (1920, 768):
        continue
    print(f"\n=== {sheet.name}  (used by kind {declared_file!r}) ===")
    if declared_facing:
        print(f"  declared per-row facing (manifest/code default): {declared_facing}")
    for r in range(4):
        vals = []
        for f in range(10):
            cell = img.crop((f * CELL, r * CELL, f * CELL + CELL, r * CELL + CELL))
            vals.append(bias(cell, *LEG))
        strong = [v for v in vals if v is not None and abs(v) >= 8]
        side = "-"
        if strong:
            side = "RIGHT" if sum(strong) > 0 else "LEFT"
        nums = " ".join("  --  " if v is None else f"{v:+6.1f}" for v in vals)
        decl = declared_facing[r] if declared_facing else 1
        decl_side = "RIGHT" if decl > 0 else "LEFT"
        agree = "" if side == "-" else ("  ok" if side == decl_side else "  <== MISMATCH")
        print(f"  r{r} {ROWS[r]:<7} {nums}   measured {side:<5}{agree}")
