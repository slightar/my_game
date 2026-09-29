from PIL import Image, ImageChops, ImageStat

ROOT = r"D:\my_game\assets\enemies\mobs"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


def mirror(a):
    return a.transpose(Image.FLIP_LEFT_RIGHT)


def band(a, y0, y1):
    out = Image.new("RGBA", (192, 192), (0, 0, 0, 0))
    out.paste(a.crop((0, y0, 192, y1)), (0, y0))
    return out


def best(a, b):
    PAD = 100
    big = Image.new("L", (192 + 2 * PAD, 192 + 2 * PAD), 0)
    big.paste(a.getchannel("A"), (PAD, PAD))
    out = 9e9
    for dx in range(-80, 81, 2):
        for dy in range(-30, 31, 2):
            win = big.crop((PAD + dx, PAD + dy, PAD + dx + 192, PAD + dy + 192))
            out = min(out, ImageStat.Stat(ImageChops.difference(win, b.getchannel("A"))).mean[0])
    return out


idle_legs, move_legs = band(cell(0, 0), 125, 192), band(cell(1, 0), 125, 192)
print("LEG BAND only (most reliable facing cue).  idle=facing LEFT, move=facing RIGHT\n")
for r, name in [(1, "row1 Move"), (3, "row3 Die ")]:
    print(name)
    for c in range(10):
        f = band(cell(r, c), 125, 192)
        d_idle, d_mir_idle = best(f, idle_legs), best(f, mirror(idle_legs))
        d_move, d_mir_move = best(f, move_legs), best(f, mirror(move_legs))
        verdict = []
        verdict.append("LEFT" if d_idle < d_mir_idle else "RIGHT")
        verdict.append("RIGHT" if d_move < d_mir_move else "LEFT")
        print(f"  f{c}: idle-ref {'LEFT' if d_idle < d_mir_idle else 'RIGHT':5s} "
              f"({d_idle:5.1f}/{d_mir_idle:5.1f})  move-ref {'RIGHT' if d_move < d_mir_move else 'LEFT':5s} "
              f"({d_move:5.1f}/{d_mir_move:5.1f})  -> {'  '.join(verdict)}")
