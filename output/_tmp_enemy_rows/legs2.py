from PIL import Image, ImageDraw

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


panels = [
    ("IDLE f0  legs", cell(0, 0), False),
    ("ATTACK f2  legs (wind-up)", cell(2, 2), False),
    ("ATTACK f5  legs (bash)", cell(2, 5), False),
    ("ATTACK f5 legs MIRRORED", cell(2, 5), True),
]
Y0, Y1 = 125, 192
Z = 4
w, h = 192 * Z, (Y1 - Y0) * Z
canvas = Image.new("RGBA", (w, h * len(panels) + 26 * len(panels)), (235, 238, 236, 255))
d = ImageDraw.Draw(canvas)
for i, (label, c, mir) in enumerate(panels):
    if mir:
        c = c.transpose(Image.FLIP_LEFT_RIGHT)
    band = c.crop((0, Y0, 192, Y1)).resize((w, h), Image.NEAREST)
    gy = i * (h + 26) + 26
    canvas.alpha_composite(band, (0, gy))
    d.line([(w // 2, gy), (w // 2, gy + h)], fill=(255, 0, 0, 255), width=2)
    d.rectangle([0, gy, w - 1, gy + h - 1], outline=(0, 160, 0, 255), width=3)
    d.text((8, gy - 22), label, fill=(180, 0, 0, 255))
canvas.convert("RGB").save(OUT + r"\legs_only.png")
print("saved", canvas.size)
