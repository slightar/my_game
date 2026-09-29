"""Render the shield's attack row EXACTLY as the game draws it for one facing.

Applies the same mirroring rule the renderer uses (attack frames 4-8 are natively
right-facing; 0-3 and 9 are natively left-facing) and lays the ten frames out in
playback order with the feet line, so the pose sequence can be inspected by eye.
"""
from PIL import Image, ImageDraw

SHEET = Image.open(r"D:\my_game\assets\enemies\mobs\shield_v2.png").convert("RGBA")
CELL = 192
ZOOM = 2
OUT = r"D:\my_game\output\_tmp_verify\shield-attack-as-drawn.png"

NATIVE_RIGHT = {4, 5, 6, 7, 8}   # the table in enemy_renderer.cpp


def cell(frame):
    c = SHEET.crop((frame * CELL, 2 * CELL, frame * CELL + CELL, 3 * CELL))
    return c.resize((CELL * ZOOM, CELL * ZOOM), Image.LANCZOS)


def build(facing):
    cols, rows = 5, 2
    pad, label = 6, 22
    cw = CELL * ZOOM
    canvas = Image.new("RGB", (cols * (cw + pad) + pad, rows * (cw + label + pad) + pad + label),
                       (22, 26, 33))
    d = ImageDraw.Draw(canvas)
    for i in range(10):
        img = cell(i)
        native = 1 if i in NATIVE_RIGHT else -1
        flip = facing != native
        if flip:
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        col, row = i % cols, i // cols
        x = pad + col * (cw + pad)
        y = pad + row * (cw + label + pad)
        backdrop = Image.new("RGB", (cw, cw), (38, 44, 54))
        backdrop.paste(img, (0, 0), img)
        canvas.paste(backdrop, (x, y))
        feet = x + int(96 * ZOOM)
        d.line([(feet, y), (feet, y + cw)], fill=(255, 90, 90), width=2)
        d.text((x + 4, y + cw + 4),
               f"f{i} {'MIRRORED' if flip else 'native'}", fill=(225, 230, 238))
    d.text((pad, 2), f"guard facing {'LEFT' if facing < 0 else 'RIGHT'}  "
                     f"(red line = feet, attack row playback order)",
           fill=(255, 220, 120))
    return canvas


left = build(-1)
left.save(OUT)
print("wrote", OUT, left.size)

right = build(1)
right.save(r"D:\my_game\output\_tmp_verify\shield-attack-as-drawn-right.png")
print("wrote right-facing version")
