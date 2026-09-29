"""Clean-export the stage-select sprites and ASCII-render them for shape reading.

The session cannot display images, so sprite shape is recovered numerically:
opaque-alpha density mapped to characters. This is what tells us whether `line`
is a vertical connector, what `bkg_normal`'s frame looks like, and so on.

Usage: python output/_tmp_new/export_stage_ui.py
"""
from pathlib import Path

import UnityPy
from UnityPy.helpers import CompressionHelper

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from extract_client_ui import decode  # noqa: E402  (registers flag-4 decoder)

CompressionHelper.DECOMPRESSION_MAP[4] = decode

ROOT = next(Path("C:/Users/34844").glob(
    "*/Arknights bilibili/games/Arknights/Arknights_Data/StreamingAssets/AB/Windows"))
OUT = Path("output/_tmp_new/stage-ui-sprites")
OUT.mkdir(parents=True, exist_ok=True)

RAMP = " .:-=+*#%@"


def ascii_map(image, cols=64):
    """Alpha-density ASCII render, preserving aspect via 2:1 char cells."""
    w, h = image.size
    rows = max(1, int(h / w * cols * 0.5))
    small = image.convert("RGBA").resize((cols, rows))
    lines = []
    for y in range(rows):
        row = []
        for x in range(cols):
            a = small.getpixel((x, y))[3]
            row.append(RAMP[min(len(RAMP) - 1, a * len(RAMP) // 256)])
        lines.append("".join(row))
    return lines


def main() -> int:
    env = UnityPy.load(str(ROOT / "ui/[uc]stage.ab"))
    saved: dict[str, tuple[int, int]] = {}
    for obj in env.objects:
        if obj.type.name not in ("Sprite", "Texture2D"):
            continue
        try:
            item = obj.read()
            name = item.m_Name
            safe = "".join(c if c.isalnum() or c in "_-" else "_" for c in name)
            prefix = "sprite" if obj.type.name == "Sprite" else "tex"
            path = OUT / f"{prefix}__{safe}.png"
            if path.exists():
                path = OUT / f"{prefix}__{safe}_{obj.path_id}.png"
            item.image.save(path)
            saved[path.name] = item.image.size
        except Exception as exc:  # noqa: BLE001
            print("SKIP", obj.path_id, str(exc)[:80])

    print(f"saved {len(saved)} files to {OUT}")
    interesting = ["line", "line_spst", "bkg_normal", "bkg_normal_branch", "bkg_hilight",
                   "bkg_training", "icon_stage_rank_3", "icon_lock", "sprite_track_point_frame",
                   "sprite_track_point_center", "details_bg", "img_dot_selected",
                   "icon_train_finish", "icon_train_unfinish", "bkg_char", "dark"]
    for want in interesting:
        matches = [n for n in saved if n.startswith("sprite__") and want in n]
        for m in sorted(matches):
            from PIL import Image
            im = Image.open(OUT / m)
            print(f"\n--- {m} {im.size} ---")
            for line in ascii_map(im):
                print("   " + line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
