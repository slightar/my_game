"""Do the baked shield cells match the original skeleton, or the MIRRORED skeleton?

The bake script rendered the skeleton un-mirrored, so a baked cell should match the
raw skeleton render and NOT its mirror. If a cell matches the mirror instead, then the
cell really was authored the other way round and the manifest facing for it is wrong.
This uses the author's own .skel as ground truth, so it is not circular.
"""
import os
from PIL import Image, ImageChops, ImageStat

BAKED = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
RAW = r"D:\my_game\output\_tmp_verify\raw_render\png"
CELL = 192


def load(p):
    return Image.open(p).convert("RGBA")


def alpha(img):
    return img.getchannel("A")


def score(a, b):
    """Mean absolute alpha difference; lower = more similar."""
    d = ImageChops.difference(alpha(a), alpha(b))
    return ImageStat.Stat(d).mean[0]


print("Compare each BAKED shield cell against the RAW skeleton render and its mirror.")
print("(raw is what the bake script drew; a match to raw = manifest facing correct)\n")
baked = load(BAKED)
print("row  frame |  vs-raw  vs-mirror | verdict")
for row in range(4):
    for frame in range(10):
        cell = baked.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
        rp = os.path.join(RAW, "r%df%d.png" % (row, frame))
        if not os.path.exists(rp):
            continue
        raw = load(rp)
        mirrored = raw.transpose(Image.FLIP_LEFT_RIGHT)
        s_raw, s_mir = score(cell, raw), score(cell, mirrored)
        verdict = "raw (facing as authored)" if s_raw < s_mir else "MIRRORED"
        print("  %d   f%-2d   |  %6.2f    %6.2f  | %s" % (row, frame, s_raw, s_mir, verdict))

print("\nSetup pose (no animation) - the author's unambiguous statement of direction:")
sp = os.path.join(RAW, "setup.png")
if os.path.exists(sp):
    im = load(sp)
    a = alpha(im).load()
    w, h = im.size
    sx = n = 0
    minx, maxx = w, -1
    for y in range(h):
        for x in range(w):
            if a[x, y] > 40:
                sx += x
                n += 1
                minx = min(minx, x)
                maxx = max(maxx, x)
    if n:
        print("  bbox x %d..%d   centroid-bboxcentre %+.1f" % (minx, maxx, sx / n - (minx + maxx) / 2))
