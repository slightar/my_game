from PIL import Image, ImageDraw

src = Image.open(r"D:\my_game\cmake-build-debug\shield-probe.png").convert("RGB")
OUT = r"D:\my_game\output\_tmp_enemy_rows"

labels = ["IDLE", "MOVE", "WINDUP", "STRIKE", "RECOVER", "TURNING"]
feet = [130 + 228 * i for i in range(6)]
centres = [310, 880]
HALF_W, TOP, BOT = 150, 215, 45
Z = 2

cellw, cellh = HALF_W * 2 * Z, (TOP + BOT) * Z
canvas = Image.new("RGB", (cellw * 2, cellh * 6), (255, 255, 255))
d = ImageDraw.Draw(canvas)
for r, label in enumerate(labels):
    for c, cx in enumerate(centres):
        box = (cx - HALF_W, feet[r] - TOP, cx + HALF_W, feet[r] + BOT)
        crop = src.crop(box).resize((cellw, cellh), Image.NEAREST)
        canvas.paste(crop, (c * cellw, r * cellh))
        d.rectangle([c * cellw, r * cellh, (c + 1) * cellw - 1, (r + 1) * cellh - 1],
                    outline=(0, 90, 220), width=2)
        d.text((c * cellw + 8, r * cellh + 6),
               f"{label}  facing={'LEFT (raw art)' if c == 0 else 'RIGHT (mirrored)'}",
               fill=(200, 0, 0))
canvas.save(OUT + r"\probe_cells.png")
print(canvas.size)
