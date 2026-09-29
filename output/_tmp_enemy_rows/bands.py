from PIL import Image, ImageChops, ImageStat

ROOT = r"D:\my_game\assets\enemies\mobs"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


def mirror(a):
    return a.transpose(Image.FLIP_LEFT_RIGHT)


def region(a, y0, y1):
    out = Image.new("RGBA", (192, 192), (0, 0, 0, 0))
    out.paste(a.crop((0, y0, 192, y1)), (0, y0))
    return out


def best(a, b):
    PAD = 100
    big = Image.new("L", (192 + 2 * PAD, 192 + 2 * PAD), 0)
    big.paste(a.getchannel("A"), (PAD, PAD))
    out = 9e9
    for dx in range(-70, 71, 2):
        for dy in range(-40, 41, 2):
            win = big.crop((PAD + dx, PAD + dy, PAD + dx + 192, PAD + dy + 192))
            out = min(out, ImageStat.Stat(ImageChops.difference(win, b.getchannel("A"))).mean[0])
    return out


idle = cell(0, 0)
move = cell(1, 0)
print("reference: idle clip (natively LEFT), move clip (natively RIGHT)")
print("compare each ATTACK frame, split into body bands (head 0-70, torso 70-130, legs 130-192)\n")
for c in range(10):
    f = cell(2, c)
    row = []
    for (y0, y1, name) in [(0, 70, "head "), (70, 130, "torso"), (130, 192, "legs ")]:
        fa, idr, mvr = region(f, y0, y1), region(idle, y0, y1), region(move, y0, y1)
        same = best(fa, idr)
        mir = best(fa, mirror(idr))
        row.append(f"{name}: idle {same:5.1f} / mir {mir:5.1f} -> {'SAME ' if same < mir else 'MIRROR'}")
    print(f"f{c}: " + " | ".join(row))
