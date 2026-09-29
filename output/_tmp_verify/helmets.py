"""High-magnification helmet comparison, all NATIVE (no mirroring applied).

Helmet front/back asymmetry (flat visor side vs rounded back) is the cue.  Compare rows
whose facing the manifest declares, against the attack frames, so we can see directly
which attack frames were authored the same way as the idle clip and which as the walk.
"""
from PIL import Image, ImageDraw

SHEET = Image.open(r"D:\my_game\assets\enemies\mobs\shield_v2.png").convert("RGBA")
CELL = 192
TOP, BOT = 4, 76
ZOOM = 4
W = CELL * ZOOM
H = (BOT - TOP) * ZOOM

ITEMS = [
    ("r0 f0  Idle  (manifest native LEFT)", 0, 0),
    ("r1 f0  Walk  (manifest native RIGHT)", 1, 0),
    ("r2 f0  Attack nat (= idle pose)", 2, 0),
    ("r2 f3  Attack nat (windup last)", 2, 3),
    ("r2 f4  Attack nat (bash first)", 2, 4),
    ("r2 f5  Attack nat (bash mid)", 2, 5),
    ("r2 f7  Attack nat (bash tail)", 2, 7),
    ("r2 f9  Attack nat (= idle pose)", 2, 9),
]

canvas = Image.new("RGB", (W + 360, len(ITEMS) * (H + 6) + 22), (22, 26, 33))
d = ImageDraw.Draw(canvas)
d.text((4, 4), "helmets, all NATIVE (no mirroring)", fill=(255, 220, 120))
for i, (label, row, frame) in enumerate(ITEMS):
    img = SHEET.crop((frame * CELL, row * CELL + TOP, frame * CELL + CELL, row * CELL + BOT))
    img = img.resize((W, H), Image.LANCZOS)
    y = 22 + i * (H + 6)
    backdrop = Image.new("RGB", (W, H), (44, 50, 60))
    backdrop.paste(img, (0, 0), img)
    canvas.paste(backdrop, (0, y))
    d.text((W + 8, y + H // 2 - 6), label, fill=(225, 230, 238))
    # centre guide
    d.line([(W // 2, y), (W // 2, y + H)], fill=(255, 90, 90), width=1)

canvas.save(r"D:\my_game\output\_tmp_verify\shield-helmets-native.png")
print("wrote shield-helmets-native.png", canvas.size)
