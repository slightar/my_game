"""Calibrated head strip.

Ground truth: the manifest says row 0 (Idle) is natively LEFT and row 1 (Walk) is
natively RIGHT.  Drawing row 0 as-is and row 1 mirrored therefore both yield a guard
that faces LEFT -- those two are the calibration for "what LEFT looks like".

Then we place the attack frames as the current code draws them (frames 4-8 mirrored)
next to an unmirrored bash for contrast, and see which ones match the calibration.
"""
from PIL import Image, ImageDraw

SHEET = Image.open(r"D:\my_game\assets\enemies\mobs\shield_v2.png").convert("RGBA")
CELL = 192
TOP, BOT = 6, 84
ZOOM = 2
W = CELL * ZOOM
H = (BOT - TOP) * ZOOM

# (label, row, frame, mirror?)
ITEMS = [
    ("REF idle  r0f0  as-is      (known LEFT)", 0, 0, False),
    ("REF walk  r1f0  mirrored   (known LEFT)", 1, 0, True),
    ("REF walk  r1f3  mirrored   (known LEFT)", 1, 3, True),
    ("atk r2f0 as drawn (nat)                 ", 2, 0, False),
    ("atk r2f5 as drawn (mir)                 ", 2, 5, True),
    ("atk r2f8 as drawn (mir)                 ", 2, 8, True),
    ("atk r2f9 as drawn (nat)                 ", 2, 9, False),
    ("--- contrast: r2f5 NATIVE (pre-fix pose)", 2, 5, False),
]

canvas = Image.new("RGB", (W + 330, len(ITEMS) * (H + 4) + 20), (22, 26, 33))
d = ImageDraw.Draw(canvas)
d.text((4, 4), "which way is the visor?  (all rows should agree)", fill=(255, 220, 120))
for i, (label, row, frame, mir) in enumerate(ITEMS):
    img = SHEET.crop((frame * CELL, row * CELL + TOP, frame * CELL + CELL, row * CELL + BOT))
    if mir:
        img = img.transpose(Image.FLIP_LEFT_RIGHT)
    img = img.resize((W, H), Image.LANCZOS)
    y = 20 + i * (H + 4)
    backdrop = Image.new("RGB", (W, H), (44, 50, 60))
    backdrop.paste(img, (0, 0), img)
    canvas.paste(backdrop, (0, y))
    d.text((W + 6, y + H // 2 - 6), label, fill=(225, 230, 238))

canvas.save(r"D:\my_game\output\_tmp_verify\shield-head-calibration.png")
print("wrote shield-head-calibration.png", canvas.size)
