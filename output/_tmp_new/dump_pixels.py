"""Per-pixel ASCII dump (1 char == 1 pixel) + alpha profiles for small sprites.

Downsampling hid the real shapes; at 1:1 we can read exactly what each sprite is,
including RGB so we can tell a frame from a gradient bar.
"""
from pathlib import Path

from PIL import Image

OUT = Path("output/_tmp_new/stage-ui-sprites")
RAMP = " .:-=+*#%@"


def dump(name: str, show_rgb: bool = False) -> None:
    path = OUT / f"sprite__{name}.png"
    if not path.is_file():
        print(f"missing {path}")
        return
    im = Image.open(path).convert("RGBA")
    w, h = im.size
    px = im.load()
    print(f"\n=== {name}  {w}x{h} ===")
    print("    " + "".join(str(x % 10) for x in range(w)))
    for y in range(h):
        row = []
        for x in range(w):
            r, g, b, a = px[x, y]
            row.append(RAMP[min(len(RAMP) - 1, a * len(RAMP) // 256)])
        print(f"{y:3d} " + "".join(row))
    if show_rgb:
        print("  -- rgb of opaque pixels (sampled) --")
        seen = {}
        for y in range(h):
            for x in range(w):
                r, g, b, a = px[x, y]
                if a > 200:
                    key = (r // 16 * 16, g // 16 * 16, b // 16 * 16)
                    seen[key] = seen.get(key, 0) + 1
        for key, n in sorted(seen.items(), key=lambda kv: -kv[1])[:8]:
            print(f"     rgb~{key}  x{n}")
    bbox = im.getchannel("A").getbbox()
    print(f"  alpha bbox = {bbox}")
    hist = im.getchannel("A").histogram()
    print(f"  alpha==0 px = {hist[0]} / {w*h}   alpha>=200 px = {sum(hist[200:])}")


for n in ["line", "sprite_track_point_frame", "icon_lock", "img_dot_selected"]:
    dump(n, show_rgb=True)
dump("bkg_normal", show_rgb=True)
