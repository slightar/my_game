from PIL import Image, ImageDraw

ROOT = r"D:\my_game\assets\enemies\mobs"
OUT = r"D:\my_game\output\_tmp_enemy_rows"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")
SCALE = 3
S = 192 * SCALE
cells = [(2, 3), (2, 4), (2, 5), (2, 6)]
grid = Image.new("RGBA", (S * 2, 2 * (S + 24)), (235, 238, 236, 255))
d = ImageDraw.Draw(grid)
for i, (row, col) in enumerate(cells):
    cell = img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))
    cell = cell.resize((S, S), Image.NEAREST)
    gx, gy = (i % 2) * S, (i // 2) * (S + 24)
    grid.alpha_composite(cell, (gx, gy + 24))
    d.text((gx + 8, gy + 4), f"attack row2 frame{col}", fill=(200, 0, 0, 255))
    d.line([(gx + S // 2, gy + 24), (gx + S // 2, gy + 24 + S)], fill=(255, 0, 0, 255), width=2)
grid.convert("RGB").save(OUT + r"\shield_attack_zoom.png")
print("saved")
