from PIL import Image, ImageDraw
import os

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
os.makedirs(OUT, exist_ok=True)

for name in ["shield_v2", "drone", "soldier", "slug", "crossbow", "exploder"]:
    p = os.path.join(ROOT, name + ".png")
    if not os.path.exists(p):
        continue
    img = Image.open(p).convert("RGBA")
    print(name, img.size)

# Shield: build an annotated contact sheet. 10 cols x 4 rows.
img = Image.open(os.path.join(ROOT, "shield_v2.png")).convert("RGBA")
W, H = img.size
for r in range(4):
    strip = img.crop((0, r * 192, W, (r + 1) * 192)).convert("RGBA")
    # composite over light background so transparent regions are visible
    bg = Image.new("RGBA", strip.size, (235, 238, 236, 255))
    bg.alpha_composite(strip)
    d = ImageDraw.Draw(bg)
    for c in range(10):
        d.line([(c * 192 + 95, 0), (c * 192 + 95, 191)], fill=(255, 0, 0, 255), width=2)
        d.line([(c * 192, 0), (c * 192, 191)], fill=(0, 0, 255, 255), width=1)
    bg.convert("RGB").save(os.path.join(OUT, f"shield_row{r}_annotated.png"))

# A compact direction probe: for every frame, report the alpha-weighted centroid x
# relative to the 192px cell centre, plus the leftmost/rightmost opaque columns.
print("\nframe-by-frame alpha centroid (offset from cell centre, px)")
for r in range(4):
    row = []
    for c in range(10):
        cell = img.crop((c * 192, r * 192, c * 192 + 192, (r + 1) * 192))
        a = cell.getchannel("A")
        px = a.load()
        tot = 0.0
        wsum = 0.0
        lo, hi = None, None
        for y in range(192):
            for x in range(192):
                v = px[x, y]
                if v > 24:
                    tot += v
                    wsum += v * x
                    lo = x if lo is None else min(lo, x)
                    hi = x if hi is None else max(hi, x)
        if tot == 0:
            row.append("empty")
        else:
            row.append(f"{wsum/tot-96:+.0f}[{lo},{hi}]")
    print(f"row{r}: " + "  ".join(row))
