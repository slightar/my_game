from PIL import Image, ImageChops, ImageStat

ROOT = r"D:\my_game\assets\enemies\mobs"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


def mirror(a):
    return a.transpose(Image.FLIP_LEFT_RIGHT)


def best(a, b):
    """Min mean-abs alpha difference over a coarse 2D search."""
    PAD = 110
    big = Image.new("L", (192 + 2 * PAD, 192 + 2 * PAD), 0)
    big.paste(a.getchannel("A"), (PAD, PAD))
    out = 9e9
    for dx in range(-80, 81, 2):
        for dy in range(-50, 51, 2):
            win = big.crop((PAD + dx, PAD + dy, PAD + dx + 192, PAD + dy + 192))
            out = min(out, ImageStat.Stat(ImageChops.difference(win, b.getchannel("A"))).mean[0])
    return out


move = cell(1, 0)          # walking clip, natively faces RIGHT (manifest row 1 = +1)
idle = cell(0, 0)          # idle clip, natively faces LEFT  (manifest row 0 = -1)
print("self-mirror difference: idle %.2f | move %.2f" %
      (ImageStat.Stat(ImageChops.difference(idle.getchannel("A"), mirror(idle).getchannel("A"))).mean[0],
       ImageStat.Stat(ImageChops.difference(move.getchannel("A"), mirror(move).getchannel("A"))).mean[0]))
print("(a large self-mirror value means the pose has a readable facing)\n")

print("attack row: distance to the RIGHT-facing walk pose vs its mirror")
print(f"{'frame':12s} {'vs move(raw=RIGHT)':>18s} {'vs mirror(move=LEFT)':>21s}   verdict")
for c in range(10):
    f = cell(2, c)
    r = best(f, move)
    l = best(f, mirror(move))
    print(f"attack f{c:<5d} {r:18.2f} {l:21.2f}   {'RIGHT' if r < l else 'LEFT '}")
