"""Derive per-enemy spriteSize / height from the baked art instead of guessing.

The baker renders every rig at scale = min(174/unionW, 164/unionH) spine-units->px, so a
model's TRUE size in spine units is (ink px in the sheet) / scale. Those units are shared
across rigs (every humanoid lands at 355-410, every originium slug at 190-245), which is
what makes relative proportions meaningful.

Anchor: the playable operator is drawn 112px tall (assets/characters/demo_guard.json),
so 士兵 - an ordinary human - is pinned to 110px and everything else follows.
"""
import json
from pathlib import Path

from PIL import Image

ROOT = Path(r"D:\my_game\assets\enemies\mobs")

# target id -> (sheet, scale)   scale comes from the bake manifest, per model.
# Every sheet now lives in ROOT under its final install name; slug_high.png *is*
# enemy_1021_bslime and irr_slug.png *is* enemy_1352_eslime, so the earlier
# exploder.png / slug_high.png indirection is gone.
SHEETS = {
    "slug": (ROOT / "slug.png", 0.6073149641541746),
    "acid_slug": (ROOT / "acid_slug.png", 0.5049686),
    "slug_high": (ROOT / "slug_high.png", 0.3379107629835345),      # enemy_1021_bslime
    "irr_slug": (ROOT / "irr_slug.png", 0.4121316936102562),        # enemy_1352_eslime
    "soldier": (ROOT / "soldier.png", 0.29430868379794184),
    "crossbow": (ROOT / "crossbow.png", 0.3143970827036548),
    "caster": (ROOT / "caster.png", 0.4140078),
    "hound": (ROOT / "hound.png", 0.5260189),
    "stealth_crossbow": (ROOT / "stealth_crossbow.png", 0.3129875),
    "shield": (ROOT / "shield_v2.png", 0.4181110759942779),
    "drone": (ROOT / "drone.png", 0.3289101333501703),
}

ANCHOR_ID = "soldier"
ANCHOR_PX = 110.0

rows = []
for key, (path, scale) in SHEETS.items():
    alpha = Image.open(path).getchannel("A")
    # Idle row = row 0. Height from the union of the row, width from the widest single cell.
    idle = alpha.crop((0, 0, 1920, 192)).getbbox()
    idle_h = idle[3] - idle[1] if idle else 0
    cell_w = 0
    for col in range(10):
        b = alpha.crop((col * 192, 0, col * 192 + 192, 192)).getbbox()
        if b:
            cell_w = max(cell_w, b[2] - b[0])
    true_h = idle_h / scale
    true_w = cell_w / scale
    rows.append({"key": key, "scale": scale, "idle_h": idle_h, "cell_w": cell_w,
                 "true_h": true_h, "true_w": true_w})

anchor = next(r for r in rows if r["key"] == ANCHOR_ID)
C = ANCHOR_PX / anchor["true_h"]          # px of screen per spine unit

print(f"anchor: {ANCHOR_ID} trueH={anchor['true_h']:.1f} -> {ANCHOR_PX}px  =>  C={C:.5f} px/unit\n")
print(f"{'id':<18}{'scale':>9}{'trueH':>8}{'trueW':>8}{'spriteSize':>11}{'height':>8}{'width':>7}")
print("-" * 72)
out = {}
for r in sorted(rows, key=lambda x: -x["true_h"]):
    sprite = C * 192 / r["scale"]
    height = C * r["true_h"]
    width = r["true_w"] / 192 * sprite
    out[r["key"]] = {"spriteSize": round(sprite), "height": round(height), "width": round(width)}
    print(f"{r['key']:<18}{r['scale']:>9.4f}{r['true_h']:>8.1f}{r['true_w']:>8.1f}"
          f"{sprite:>11.1f}{height:>8.1f}{width:>7.1f}")

print("\n--- values for enemy_data.cpp ---")
print("height / spriteSize go straight in; the `width` field in that table is a")
print("hand-tuned HITBOX and is deliberately NOT taken from here (the rig width")
print("below includes outstretched arms/weapons and would be far too broad).")
for k, v in out.items():
    print(f'{k:<18} height {v["height"]:>4d}   spriteSize {v["spriteSize"]:>4d}'
          f'   (rig width {v["width"]:>4d}, unused)')
Path(r"D:\my_game\output\_tmp_new\size_table.json").write_text(
    json.dumps(out, indent=2), encoding="utf-8")
