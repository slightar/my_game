from PIL import Image, ImageDraw

src = Image.open(r"D:\my_game\cmake-build-debug\shield-probe.png").convert("RGB")
OUT = r"D:\my_game\output\_tmp_enemy_rows"

# rows of interest: idle (0), windup (2), strike (3)
picks = [(0, "IDLE"), (2, "WINDUP"), (3, "STRIKE"), (4, "RECOVER")]
feet = [130 + 228 * i for i in range(6)]
centres = [310, 880]
HALF_W, TOP, BOT = 150, 215, 45
Z = 2
cellw, cellh = HALF_W * 2 * Z, (TOP + BOT) * Z

canvas = Image.new("RGB", (cellw * 2, cellh * len(picks)), (255, 255, 255))
d = ImageDraw.Draw(canvas)
for r, (idx, label) in enumerate(picks):
    for c, cx in enumerate(centres):
        crop = src.crop((cx - HALF_W, feet[idx] - TOP, cx + HALF_W, feet[idx] + BOT))
        crop = crop.resize((cellw, cellh), Image.NEAREST)
        canvas.paste(crop, (c * cellw, r * cellh))
        d.rectangle([c * cellw, r * cellh, (c + 1) * cellw - 1, (r + 1) * cellh - 1],
                    outline=(0, 90, 220), width=2)
        d.text((c * cellw + 10, r * cellh + 8),
               f"{label} | facing={'LEFT  raw art' if c == 0 else 'RIGHT  mirrored'}",
               fill=(200, 0, 0))
canvas.save(OUT + r"\probe_key_rows.png")
print(canvas.size)
