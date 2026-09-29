#!/usr/bin/env python
"""Per-frame view of the shield attack row (row 2), native vs mirrored.

Writes one image per group of three frames so each cell stays readable.
"""
from PIL import Image, ImageDraw

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
OUT_DIR = r"D:\my_game\output\_tmp_verify"
CELL = 192
SCALE = 2

sheet = Image.open(SHEET).convert("RGBA")
cell_px = CELL * SCALE
label_h = 30
gap = 8

groups = {"f0-f3": [0, 1, 2, 3], "f4-f6": [4, 5, 6], "f7-f9": [7, 8, 9]}

for tag, frames in groups.items():
    cols = len(frames)
    rows = 2  # native on top, mirrored below
    canvas = Image.new(
        "RGB",
        (cols * cell_px + (cols + 1) * gap, rows * (cell_px + label_h) + (rows + 1) * gap),
        (22, 26, 34),
    )
    draw = ImageDraw.Draw(canvas)
    for row_slot, mirror in ((0, False), (1, True)):
        for col, frame in enumerate(frames):
            cell = sheet.crop((frame * CELL, 2 * CELL, frame * CELL + CELL, 3 * CELL))
            image = cell.resize((cell_px, cell_px), Image.LANCZOS)
            if mirror:
                image = image.transpose(Image.FLIP_LEFT_RIGHT)
            backdrop = Image.new("RGB", (cell_px, cell_px), (40, 46, 56))
            backdrop.paste(image, (0, 0), image)
            x = gap + col * (cell_px + gap)
            y = gap + row_slot * (cell_px + label_h + gap)
            canvas.paste(backdrop, (x, y))
            draw.text((x + 6, y - label_h + 8),
                      f"row2 frame {frame}  {'MIRRORED' if mirror else 'native'}",
                      fill=(232, 234, 238))
            # Cell centre marker: which side the shield mass sits on.
            centre = x + cell_px // 2
            draw.line([(centre, y), (centre, y + cell_px)], fill=(255, 96, 96), width=2)
    path = f"{OUT_DIR}\\shield-attack-{tag}.png"
    canvas.save(path)
    print(f"wrote {path} ({canvas.width}x{canvas.height})")

