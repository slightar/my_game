from PIL import Image, ImageDraw

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")
SCALE = 3
S = 192 * SCALE

# 2x2 grid: idle f0, move f0, attack f0, attack f5
cells = [(0, 0, "idle r0 f0"), (1, 0, "move r1 f0"), (2, 0, "attack r2 f0"), (2, 5, "attack r2 f5")]
grid = Image.new("RGBA", (S * 2, S * 2), (235, 238, 236, 255))
d = ImageDraw.Draw(grid)
for i, (row, col, label) in enumerate(cells):
    cell = img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))
    cell = cell.resize((S, S), Image.NEAREST)
    gx, gy = (i % 2) * S, (i // 2) * S
    grid.alpha_composite(cell, (gx, gy))
    d.line([(gx + S // 2, gy), (gx + S // 2, gy + S)], fill=(255, 0, 0, 255), width=2)
    d.rectangle([gx + 1, gy + 1, gx + S - 2, gy + S - 2], outline=(0, 0, 255, 255), width=2)
    d.text((gx + 8, gy + 6), label, fill=(200, 0, 0, 255))
grid.convert("RGB").save(OUT + r"\shield_zoom_grid.png")
print("saved")
