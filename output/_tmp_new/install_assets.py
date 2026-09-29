"""Install the corrected enemy sheets and rebuild manifest.json for the 11 official mobs.

Renames are ordered so the eslime sheet vacates `slug_high.png` before bslime claims it:
  exploder.png (bslime)      -> slug_high.png   (高能源石虫)
  slug_high.png (eslime)     -> irr_slug.png    (辐能源石虫)
"""
import json
import shutil
from pathlib import Path

MOBS = Path(r"D:\my_game\assets\enemies\mobs")
NEW = Path(r"D:\my_game\output\_tmp_new\rebake\out")

old = json.loads((MOBS / "manifest.json").read_text(encoding="utf-8"))
fresh = json.loads((NEW / "manifest.json").read_text(encoding="utf-8"))

# 1) move the eslime sheet out of the way FIRST, then let bslime take the name
if (MOBS / "slug_high.png").is_file() and not (MOBS / "irr_slug.png").is_file():
    shutil.move(str(MOBS / "slug_high.png"), str(MOBS / "irr_slug.png"))
if (MOBS / "exploder.png").is_file() and not (MOBS / "slug_high.png").is_file():
    shutil.move(str(MOBS / "exploder.png"), str(MOBS / "slug_high.png"))

# 2) copy the four freshly baked sheets
for key in ("hound", "acid_slug", "caster", "stealth_crossbow"):
    shutil.copyfile(NEW / f"{key}.png", MOBS / f"{key}.png")

# 3) rebuild the manifest: target id -> (source dict, model key)
entries = {
    "slug": (old["slug"], "enemy_1007_slime"),
    "acid_slug": (fresh["acid_slug"], "enemy_1004_mslime"),
    "slug_high": (old["exploder"], "enemy_1021_bslime"),
    "irr_slug": (old["slug_high"], "enemy_1352_eslime"),
    "soldier": (old["soldier"], "enemy_1002_nsabr"),
    "crossbow": (old["crossbow"], "enemy_1003_ncbow"),
    "caster": (fresh["caster"], "enemy_1011_wizard"),
    "hound": (fresh["hound"], "enemy_1000_gopro"),
    "stealth_crossbow": (fresh["stealth_crossbow"], "enemy_1019_jshoot"),
    "shield": (old["shield"], "enemy_1006_shield"),
    "drone": (old["drone"], "enemy_1005_yokai"),
}

manifest = {}
for key, (src, model) in entries.items():
    e = dict(src)
    e["model"] = model
    # A sheet that is not <id>.png must say so (shield keeps shield_v2.png).
    e.pop("file", None)
    if key == "shield":
        e["file"] = "shield_v2.png"
    elif key == "drone":
        e["file"] = "drone.png"
    manifest[key] = e

(MOBS / "manifest.json").write_text(
    json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

print(f"{'id':<18}{'model':<20}{'scale':>9}{'anchor':>18}{'file':>16}")
print("-" * 82)
for key, e in manifest.items():
    a = e.get("anchor", [0, 0])
    print(f"{key:<18}{e['model']:<20}{e.get('scale', 0):>9.4f}"
          f"{f'[{a[0]:.0f},{a[1]:.0f}]':>18}{e.get('file', key + '.png'):>16}")

print("\nfiles present:")
for p in sorted(MOBS.glob("*.png")):
    print(f"   {p.name:<26}{p.stat().st_size:>9d}")
