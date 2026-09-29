from PIL import Image, ImageChops, ImageStat

ROOT = r"D:\my_game\assets\enemies\mobs"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


idle0 = cell(0, 0)

# Locate the helmet in the idle pose: topmost opaque row, then take a box.
pa = idle0.getchannel("A").load()
top = next(y for y in range(192) if any(pa[x, y] > 40 for x in range(192)))
xs = [x for x in range(192) if pa[x, top] > 40]
hcx = (min(xs) + max(xs)) // 2
print(f"idle helmet top row {top}, centre x {hcx}")

TW, TH = 56, 52
template = idle0.crop((hcx - TW // 2, top - 4, hcx + TW // 2, top - 4 + TH))
mirror = template.transpose(Image.FLIP_LEFT_RIGHT)

PAD = 90


def locate(frame):
    """Slide the template (and its mirror) over the frame; return best SAD for each."""
    big = Image.new("RGBA", (192 + 2 * PAD, 192 + 2 * PAD), (0, 0, 0, 0))
    big.paste(frame, (PAD, PAD))
    best_same, best_mir = 9e9, 9e9
    for dy in range(-60, 71, 2):
        for dx in range(-70, 71, 2):
            win = big.crop((PAD + hcx - TW // 2 + dx, PAD + top - 4 + dy,
                            PAD + hcx + TW // 2 + dx, PAD + top - 4 + TH + dy))
            if ImageStat.Stat(win.getchannel("A")).mean[0] < 6:
                continue
            same = ImageStat.Stat(ImageChops.difference(win.getchannel("A"), template.getchannel("A"))).mean[0]
            m = ImageStat.Stat(ImageChops.difference(win.getchannel("A"), mirror.getchannel("A"))).mean[0]
            best_same = min(best_same, same)
            best_mir = min(best_mir, m)
    return best_same, best_mir


print(f"{'frame':10s} {'as-is':>8s} {'mirrored':>9s}   verdict (helmet found in ... orientation)")
for r in range(4):
    for c in range(10):
        if r != 2 and c not in (0, 5):
            continue
        s, m = locate(cell(r, c))
        verdict = "IDLE/left" if s < m else "MIRRORED/right"
        print(f"r{r}f{c:<8d} {s:8.2f} {m:9.2f}   {verdict}")
