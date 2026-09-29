"""Zoom the shield bash frames (contact-sheet column 2) for a side-by-side look."""
from PIL import Image

SRC = r"D:\my_game\cmake-build-debug\shield-fix-sheet.png"
OUT = r"D:\my_game\output\_tmp_verify\shield-bash-zoom.png"

sheet = Image.open(SRC).convert("RGB")
# Column 2 = Strike/bash, feet.x = 256 + 2*512 = 1280, feet.y = 400 (row 0) / 912 (row 1).
BOX_W, BOX_H = 300, 220
crops = []
for row, feet_y in ((0, 400), (1, 912)):
    box = (1280 - BOX_W // 2, feet_y - BOX_H + 40, 1280 + BOX_W // 2, feet_y + 40)
    crops.append(sheet.crop(box).resize((BOX_W * 3, BOX_H * 3), Image.NEAREST))

gap = 24
canvas = Image.new("RGB", (BOX_W * 3, BOX_H * 3 * 2 + gap), (16, 18, 22))
canvas.paste(crops[0], (0, 0))
canvas.paste(crops[1], (0, BOX_H * 3 + gap))
canvas.save(OUT)
print(f"wrote {OUT}")
print("top = facing -1 (attacks left) | bottom = facing +1 (attacks right)")
