from PIL import Image, ImageDraw

src = Image.open(r"D:\my_game\cmake-build-debug\shield-probe.png").convert("RGB")
OUT = r"D:\my_game\output\_tmp_enemy_rows"

# probe layout: rows IDLE=300, WINDUP=620, STRIKE=940, RECOVER=1260 ; centres 320 / 860
feet = {"WINDUP": 620, "STRIKE": 940}
HALF_W, TOP, BOT = 160, 200, 55
Z = 2
cw, ch = HALF_W * 2 * Z, (TOP + BOT) * Z

canvas = Image.new("RGB", (cw * 2, ch * 2), (255, 255, 255))
d = ImageDraw.Draw(canvas)
for r, state in enumerate(["WINDUP", "STRIKE"]):
    fy = feet[state]
    for c, mode in enumerate(["A", "B"]):
        cx = 320 if mode == "A" else 860
        crop = src.crop((cx - HALF_W, fy - TOP, cx + HALF_W, fy + BOT))
        if mode == "B":
            crop = crop.transpose(Image.FLIP_LEFT_RIGHT)  # mirror of facing +1 == fixed render for facing -1
        crop = crop.resize((cw, ch), Image.NEAREST)
        canvas.paste(crop, (c * cw, r * ch))
        d.rectangle([c * cw, r * ch, (c + 1) * cw - 1, (r + 1) * ch - 1], outline=(0, 90, 220), width=3)
        tag = "A  当前 (facing=-1 原样)" if mode == "A" else "B  修复后 (facing=-1 镜像)"
        d.text((c * cw + 12, r * ch + 10), f"{state}   {tag}   左下方橙条=实际命中方向(左)", fill=(200, 0, 0))
canvas.save(OUT + r"\probe_fix_ab.png")
print(canvas.size)
