from PIL import Image, ImageDraw, ImageChops, ImageStat

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


Z = 2
S = 192 * Z
panels = [
    ("idle r0 f0", cell(0, 0), False),
    ("move r1 f0", cell(1, 0), False),
    ("MIRROR of move r1 f0", cell(1, 0), True),
    ("attack r2 f0", cell(2, 0), False),
    ("attack r2 f4", cell(2, 4), False),
    ("attack r2 f5", cell(2, 5), False),
]
cols, rows = 3, 2
canvas = Image.new("RGBA", (cols * S, rows * (S + 26)), (235, 238, 236, 255))
d = ImageDraw.Draw(canvas)
for i, (label, c, mir) in enumerate(panels):
    if mir:
        c = c.transpose(Image.FLIP_LEFT_RIGHT)
    c = c.resize((S, S), Image.NEAREST)
    gx, gy = (i % cols) * S, (i // cols) * (S + 26) + 26
    canvas.alpha_composite(c, (gx, gy))
    d.line([(gx + S // 2, gy), (gx + S // 2, gy + S)], fill=(255, 0, 0, 255), width=2)
    d.rectangle([gx, gy, gx + S - 1, gy + S - 1], outline=(0, 0, 255, 255))
    d.text((gx + 8, gy - 20), label, fill=(180, 0, 0, 255))
canvas.convert("RGB").save(OUT + r"\shield_compare.png")
print("saved")

# Shift-tolerant orientation test: is the frame closer to idle or to mirrored idle,
# with the best horizontal alignment?
def best_shift_diff(a, b):
    PAD = 200
    diffs = []
    for dx in range(-90, 91, 3):
        for dy in (-40, -20, 0, 20, 40):
            canvas_a = Image.new("L", (192 + 2 * PAD, 192 + 2 * PAD), 0)
            canvas_a.paste(a.getchannel("A"), (PAD, PAD))
            ca = canvas_a.crop((PAD + dx, PAD + dy, PAD + dx + 192, PAD + dy + 192))
            diffs.append((ImageStat.Stat(ImageChops.difference(ca, b.getchannel("A"))).mean[0], dx, dy))
    return min(diffs)


for c in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]:
    f = cell(2, c)
    same, sdx, sdy = best_shift_diff(f, cell(0, 0))
    mir, mdx, mdy = best_shift_diff(f, cell(0, 0).transpose(Image.FLIP_LEFT_RIGHT))
    verdict = "SAME as idle" if same < mir else "MIRRORED vs idle"
    print(f"atk f{c}: best-vs-idle {same:6.2f} @dx{sdx:+d}  |  best-vs-mirror {mir:6.2f} @dx{mdx:+d}  -> {verdict}")
