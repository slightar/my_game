"""Show native vs mirrored shield cells so the authored facing can be judged by eye."""
from PIL import Image, ImageDraw

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
OUT = r"D:\my_game\output\_tmp_verify\shield-native-vs-mirror.png"
CELL = 192

sheet = Image.open(SHEET).convert("RGBA")

# (label, row, frame)
cells = [
    ("Idle r0 f0", 0, 0),
    ("Move r1 f0", 1, 0),
    ("Attack r2 f0", 2, 0),
    ("Attack r2 f2 (windup)", 2, 2),
    ("Attack r2 f5 (bash)", 2, 5),
    ("Attack r2 f7", 2, 7),
    ("Attack r2 f9", 2, 9),
]

SCALE = 2
CELL_PX = CELL * SCALE
LABEL_H = 26
GAP = 10
cols = 4
rows = (len(cells) + cols - 1) // cols

canvas = Image.new(
    "RGB",
    (cols * CELL_PX + (cols + 1) * GAP, rows * (CELL_PX + LABEL_H) + (rows + 1) * GAP),
    (24, 28, 36),
)
draw = ImageDraw.Draw(canvas)

mirrored_labels = {"Attack r2 f5 (bash)"}
for row_index, (label, row, frame) in enumerate(cells):
    cell = sheet.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
    native = cell.resize((CELL_PX, CELL_PX), Image.NEAREST)
    image = native.transpose(Image.FLIP_LEFT_RIGHT) if label in mirrored_labels else native
    col_index = row_index % cols
    row_slot = row_index // cols
    x = GAP + col_index * (CELL_PX + GAP)
    y = GAP + row_slot * (CELL_PX + LABEL_H + GAP)
    if label in mirrored_labels:
        label += " [MIRRORED]"
    backdrop = Image.new("RGB", (CELL_PX, CELL_PX), (36, 42, 52))
    backdrop.paste(image, (0, 0), image)
    canvas.paste(backdrop, (x, y))
    draw.text((x + 4, y - LABEL_H + 6), label, fill=(230, 232, 236))

canvas.save(OUT)
print(f"wrote {OUT}  ({canvas.width}x{canvas.height})")
