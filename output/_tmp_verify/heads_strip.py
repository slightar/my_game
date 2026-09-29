"""Head-only strip of the attack row as the game draws it, for a left-facing guard.

The helmet's visor is the clearest facing cue on this sprite.  Frames are stacked
vertically with the mirroring the renderer applies, so any frame that faces the wrong
way stands out immediately.
"""
from PIL import Image, ImageDraw

SHEET = Image.open(r"D:\my_game\assets\enemies\mobs\shield_v2.png").convert("RGBA")
CELL = 192
TOP, BOT = 6, 84
ZOOM = 2
NATIVE_RIGHT = {4, 5, 6, 7, 8}
OUT = r"D:\my_game\output\_tmp_verify\shield-attack-heads.png"

w = CELL * ZOOM
h = (BOT - TOP) * ZOOM
canvas = Image.new("RGB", (w + 130, 10 * (h + 4) + 20), (22, 26, 33))
d = ImageDraw.Draw(canvas)
d.text((4, 4), "attack row heads, guard facing LEFT", fill=(255, 220, 120))
for i in range(10):
    img = SHEET.crop((i * CELL, 2 * CELL + TOP, i * CELL + CELL, 2 * CELL + BOT))
    native = 1 if i in NATIVE_RIGHT else -1
    flip = -1 != native
    if flip:
        img = img.transpose(Image.FLIP_LEFT_RIGHT)
    img = img.resize((w, h), Image.LANCZOS)
    y = 20 + i * (h + 4)
    backdrop = Image.new("RGB", (w, h), (44, 50, 60))
    backdrop.paste(img, (0, 0), img)
    canvas.paste(backdrop, (0, y))
    d.text((w + 6, y + h // 2 - 6),
           f"f{i} {'mir' if flip else 'nat'}", fill=(225, 230, 238))

canvas.save(OUT)
print("wrote", OUT, canvas.size)
