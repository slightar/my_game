from PIL import Image, ImageDraw

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


panels = [
    ("IDLE f0  (natively LEFT)", cell(0, 0), False),
    ("ATTACK f2  (wind-up)", cell(2, 2), False),
    ("ATTACK f5  (bash)", cell(2, 5), False),
    ("ATTACK f5 MIRRORED", cell(2, 5), True),
]
Z = 3
S = 192 * Z
canvas = Image.new("RGBA", (S * 2, 2 * (S + 30)), (235, 238, 236, 255))
d = ImageDraw.Draw(canvas)
for i, (label, c, mir) in enumerate(panels):
    if mir:
        c = c.transpose(Image.FLIP_LEFT_RIGHT)
    c = c.resize((S, S), Image.NEAREST)
    gx, gy = (i % 2) * S, (i // 2) * (S + 30) + 30
    canvas.alpha_composite(c, (gx, gy))
    d.line([(gx + S // 2, gy), (gx + S // 2, gy + S)], fill=(255, 0, 0, 255), width=2)
    # highlight the leg band (opaque bbox bottom 60 px of the cell)
    d.rectangle([gx, gy + 130 * Z, gx + S - 1, gy + 191 * Z], outline=(0, 160, 0, 255), width=3)
    d.text((gx + 8, gy - 24), label, fill=(180, 0, 0, 255))
canvas.convert("RGB").save(OUT + r"\legs.png")
print("saved")
