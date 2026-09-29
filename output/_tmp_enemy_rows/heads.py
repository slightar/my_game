from PIL import Image, ImageDraw

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")

# Heads: crop a 96x80 region around the helmet for a few frames and blow it up 5x.
picks = [(0, 0, "idle f0"), (1, 0, "move f0"), (2, 0, "atk f0"), (2, 3, "atk f3"),
         (2, 4, "atk f4"), (2, 5, "atk f5"), (2, 8, "atk f8")]
Z = 4
cw, ch = 110, 80
canvas = Image.new("RGBA", (Z * cw * 4, Z * ch * 2), (235, 238, 236, 255))
d = ImageDraw.Draw(canvas)
for i, (row, col, label) in enumerate(picks):
    cell = img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))
    head = cell.crop((45, 10, 45 + cw, 10 + ch)).resize((Z * cw, Z * ch), Image.NEAREST)
    gx, gy = (i % 4) * Z * cw, (i // 4) * Z * ch
    canvas.alpha_composite(head, (gx, gy))
    d.rectangle([gx, gy, gx + Z * cw - 1, gy + Z * ch - 1], outline=(0, 0, 255, 255))
    d.text((gx + 6, gy + 4), label, fill=(200, 0, 0, 255))
canvas.convert("RGB").save(OUT + r"\shield_heads.png")
print("saved")
