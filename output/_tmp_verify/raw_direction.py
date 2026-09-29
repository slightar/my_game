"""Which way does the ORIGINAL skeleton face? Settle it on the rig, then map it back.

We measure the raw setup pose and each raw clip frame, then read the manifest facing
against the mirror findings. The aim is a single, self-consistent statement of the
native direction of every cell of shield_v2.png in the game's (-1 left / +1 right)
convention.
"""
import os
from PIL import Image

RAW = r"D:\my_game\output\_tmp_verify\raw_render\png"
BAKED = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
CELL = 192


def load(p):
    return Image.open(p).convert("RGBA")


def alpha_stats(img):
    a = img.getchannel("A").load()
    w, h = img.size
    sx = n = 0
    minx, maxx, miny, maxy = w, -1, h, -1
    for y in range(h):
        for x in range(w):
            if a[x, y] > 40:
                sx += x
                n += 1
                minx = min(minx, x)
                maxx = max(maxx, x)
                miny = min(miny, y)
                maxy = max(maxy, y)
    if not n:
        return None
    return {
        "centroid": sx / n, "bboxcx": (minx + maxx) / 2,
        "bias": sx / n - (minx + maxx) / 2,
        "x0": minx, "x1": maxx, "w": maxx - minx,
    }


def hprofile(img, top=0.60):
    """Left/right opaqueness in the UPPER part: the shield is held high on one side."""
    a = img.getchannel("A").load()
    w, h = img.size
    left = right = 0
    for y in range(int(h * top)):
        for x in range(w):
            if a[x, y] > 60:
                if x < w / 2:
                    left += 1
                else:
                    right += 1
    return left, right


print("RAW skeleton (author's model). Bias = alpha centroid - bbox centre.")
print("A shield carried on one side makes the upper silhouette lopsided.\n")
print("pose        bias   bbox        upper-left  upper-right  heavier side")
for name in ["setup"] + ["r%df%d" % (r, f) for r in range(4) for f in range(10)]:
    p = os.path.join(RAW, name + ".png")
    if not os.path.exists(p):
        continue
    im = load(p)
    s = alpha_stats(im)
    if s is None:
        continue
    l, r = hprofile(im)
    side = "LEFT" if l > r else "RIGHT"
    print("%-8s %+7.1f  x%3d..%3d  %9d  %11d  %s" % (name, s["bias"], s["x0"], s["x1"], l, r, side))

print("\nBaked shield_v2.png, same measurement:")
baked = load(BAKED)
print("cell      bias   bbox        upper-left  upper-right  heavier side")
for row in range(4):
    for frame in range(10):
        c = baked.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
        s = alpha_stats(c)
        l, r = hprofile(c)
        side = "LEFT" if l > r else "RIGHT"
        print("r%df%-6d %+7.1f  x%3d..%3d  %9d  %11d  %s" % (row, frame, s["bias"], s["x0"], s["x1"], l, r, side))
