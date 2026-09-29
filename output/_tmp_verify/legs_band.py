"""Zoom the leg band of the shield attack row so the body's facing can be judged
independently of where the shield happens to swing."""
from PIL import Image, ImageDraw

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
OUT = r"D:\my_game\output\_tmp_verify\shield-legs-band.png"
CELL = 192
# The feet/legs sit in the lower part of the 192 cell.
BAND_TOP = 120
SCALE = 4

sheet = Image.open(SHEET).convert("RGBA")
frames = [0, 1, 2, 3, 4, 5, 9]
cell_px = CELL * SCALE
band_px = (CELL - BAND_TOP) * SCALE
label_h = 28
gap = 8

cols = len(frames)
rows = 2
canvas = Image.new(
    "RGB",
    (cols * cell_px + (cols + 1) * gap, rows * (band_px + label_h) + (rows + 1) * gap),
    (20, 24, 32),
)
draw = ImageDraw.Draw(canvas)

for row_slot, mirror in ((0, False), (1, True)):
    for col, frame in enumerate(frames):
        band = sheet.crop((frame * CELL, 2 * CELL + BAND_TOP,
                           frame * CELL + CELL, 3 * CELL))
        image = band.resize((cell_px, band_px), Image.LANCZOS)
        if mirror:
            image = image.transpose(Image.FLIP_LEFT_RIGHT)
        backdrop = Image.new("RGB", (cell_px, band_px), (44, 50, 60))
        backdrop.paste(image, (0, 0), image)
        x = gap + col * (cell_px + gap)
        y = gap + row_slot * (band_px + label_h + gap)
        canvas.paste(backdrop, (x, y))
        draw.text((x + 6, y - label_h + 8),
                  f"row2 f{frame} legs  {'MIRRORED' if mirror else 'native'}",
                  fill=(232, 234, 238))
        centre = x + cell_px // 2
        draw.line([(centre, y), (centre, y + band_px)], fill=(255, 96, 96), width=2)

canvas.save(OUT)
print(f"wrote {OUT} ({canvas.width}x{canvas.height})")
print("frame 0 is the idle pose, known to be drawn facing LEFT.")
