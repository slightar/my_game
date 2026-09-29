"""Lay out the shield's idle, walk and die rows exactly as the game draws them for a
left-facing guard, so a stray frame that flips is visible at a glance."""
from PIL import Image, ImageDraw

SHEET = Image.open(r"D:\my_game\assets\enemies\mobs\shield_v2.png").convert("RGBA")
CELL = 192
ZOOM = 1
OUT = r"D:\my_game\output\_tmp_verify\shield-other-rows-as-drawn.png"

# per-row declared native facing from the manifest: [-1, 1, -1, -1]
ROW_FACING = [-1, 1, -1, -1]
ROWS = [(0, "Idle"), (1, "Walk"), (3, "Die")]
FACING = -1
PAD, LABEL = 5, 18
cw = CELL * ZOOM
canvas = Image.new("RGB", (10 * (cw + PAD) + PAD, len(ROWS) * (cw + LABEL + PAD) + PAD + LABEL),
                   (22, 26, 33))
d = ImageDraw.Draw(canvas)
d.text((PAD, 2), f"guard facing LEFT - idle / walk / die rows as drawn (red = feet)",
       fill=(255, 220, 120))
for r, (row, name) in enumerate(ROWS):
    for f in range(10):
        img = SHEET.crop((f * CELL, row * CELL, f * CELL + CELL, row * CELL + CELL))
        native = ROW_FACING[row]
        flip = FACING != native
        if flip:
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        x = PAD + f * (cw + PAD)
        y = PAD + LABEL + r * (cw + LABEL + PAD)
        backdrop = Image.new("RGB", (cw, cw), (38, 44, 54))
        backdrop.paste(img, (0, 0), img)
        canvas.paste(backdrop, (x, y))
        d.line([(x + 96 * ZOOM, y), (x + 96 * ZOOM, y + cw)], fill=(255, 90, 90), width=1)
    d.text((PAD, PAD + LABEL + r * (cw + LABEL + PAD) + cw + 2),
           f"row {row} {name}   (native facing {native}, flip={FACING != native})",
           fill=(225, 230, 238))

canvas.save(OUT)
print("wrote", OUT, canvas.size)
