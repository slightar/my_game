from PIL import Image

ROOT = r"D:\my_game\assets\enemies\mobs"


def load(name):
    return Image.open(ROOT + "\\" + name + ".png").convert("RGBA")


def cell(img, row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


def visor_offset(c):
    """Find the helmet: top band of the opaque bbox. The visor slits are the
    brightest near-white pixels there. Return visor x-centroid minus head
    bbox centre x (positive = visor on the right = character faces right)."""
    px = c.load()
    xs, ys = [], []
    for y in range(192):
        for x in range(192):
            if px[x, y][3] > 40:
                xs.append(x)
                ys.append(y)
    if not xs:
        return None
    top = min(ys)
    head_bottom = top + 62  # helmet band
    hx = [x for x, y in zip(xs, ys) if y <= head_bottom]
    if not hx:
        return None
    hc = (min(hx) + max(hx)) / 2
    bright = [(x, y) for x, y in zip(xs, ys)
              if y <= head_bottom and min(px[x, y][:3]) > 140]
    if not bright:
        return None
    vc = sum(x for x, _ in bright) / len(bright)
    return round(vc - hc, 1), len(bright), round(hc, 1)


print("calibration / shield_v2")
sh = load("shield_v2")
rows = {0: "idle", 1: "move", 2: "attack", 3: "die"}
for r in range(4):
    line = []
    for c in range(10):
        res = visor_offset(cell(sh, r, c))
        line.append("none" if res is None else f"{res[0]:+.0f}")
    print(f"  row{r} {rows[r]:7s}: " + "  ".join(line))

print("\ncalibration / soldier (manifest says every row faces RIGHT = +1)")
so = load("soldier")
for r in range(4):
    line = []
    for c in [0, 4, 8]:
        res = visor_offset(cell(so, r, c))
        line.append("none" if res is None else f"{res[0]:+.0f}")
    print(f"  row{r}: " + "  ".join(line))

print("\ncalibration / crossbow (manifest says every row faces RIGHT = +1)")
cb = load("crossbow")
for r in range(4):
    line = []
    for c in [0, 4, 8]:
        res = visor_offset(cell(cb, r, c))
        line.append("none" if res is None else f"{res[0]:+.0f}")
    print(f"  row{r}: " + "  ".join(line))
