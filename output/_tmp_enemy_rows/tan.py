from PIL import Image

ROOT = r"D:\my_game\assets\enemies\mobs"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


def tan_centroid(c):
    """The soldier's khaki/tan gear is a strong, colour-based facing cue."""
    px = c.load()
    xs = []
    for y in range(192):
        for x in range(192):
            r, g, b, a = px[x, y]
            if a > 120 and r > 105 and g > 95 and b < 105 and r > b + 35 and g > b + 20 and abs(r - g) < 45:
                xs.append(x)
    if len(xs) < 30:
        return None, len(xs)
    return sum(xs) / len(xs) - 96, len(xs)


print("khaki-pixel centroid offset from the cell centre (negative = gear on the LEFT)")
for r, name in [(0, "row0 Idle"), (1, "row1 Move"), (2, "row2 ATTACK"), (3, "row3 Die ")]:
    parts = []
    for c in range(10):
        v, n = tan_centroid(cell(r, c))
        parts.append(f"f{c}:" + ("n/a" if v is None else f"{v:+.0f}"))
    print(f"{name}: " + "  ".join(parts))
