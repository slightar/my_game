"""Which way do the FEET point? A front-on pose still has toed-out feet.

The shield's idle reads as near front-on, so leaning and body mass cannot tell us
the authored direction. The foot of the leading leg, however, is a wedge: its long
axis points where the character is facing. For each foot we take the connected
component in the feet band and report the signed slope of its bottom edge, plus the
horizontal flank that rises steeply (the toe) versus the vertical flank (the heel).
"""
import os
from PIL import Image

BASE = r"D:\my_game\assets\enemies\mobs"
CELL = 192


def components(mask, w, h):
    seen = bytearray(w * h)
    out = []
    for start in range(w * h):
        if seen[start] or not mask[start]:
            continue
        stack = [start]
        seen[start] = 1
        pts = []
        while stack:
            p = stack.pop()
            pts.append(p)
            y, x = divmod(p, w)
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < h and 0 <= nx < w:
                        q = ny * w + nx
                        if not seen[q] and mask[q]:
                            seen[q] = 1
                            stack.append(q)
        if len(pts) > 12:
            out.append(pts)
    return out


def feet_report(sheet, row, col, band_frac=0.30):
    im = Image.open(os.path.join(BASE, sheet)).convert("RGBA")
    cell = im.crop((col * CELL, row * CELL, col * CELL + CELL, row * CELL + CELL))
    a = cell.getchannel("A").load()
    miny = maxy = None
    for y in range(CELL):
        for x in range(CELL):
            if a[x, y] > 60:
                if miny is None:
                    miny = y
                maxy = y
    top = int(miny + (maxy - miny) * (1 - band_frac))
    w = CELL
    h = CELL - top
    mask = [1 if a[x, top + y] > 60 else 0 for y in range(h) for x in range(w)]
    comps = components(mask, w, h)
    comps.sort(key=lambda p: -len(p))
    print("%s row%d f%d  band y=%d..%d  components=%d" % (sheet, row, col, top, maxy, len(comps)))
    for ci, pts in enumerate(comps[:3]):
        xs = [divmod(p, w)[1] for p in pts]
        ys = [divmod(p, w)[0] for p in pts]
        x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
        # bottom edge: for each column take the lowest opaque row -> its signed slope
        bottom = {}
        for p in pts:
            y, x = divmod(p, w)
            bottom[x] = max(bottom.get(x, -1), y)
        cols = sorted(bottom)
        n = len(cols)
        # slope of the bottom profile = toe raise direction
        lh = sum(bottom[cols[i]] for i in range(n // 3)) / max(1, n // 3)
        rh = sum(bottom[cols[i]] for i in range(n - n // 3, n)) / max(1, n - n // 3)
        # height profile: which flank is tall (heel) vs low (toe)
        toph = {}
        for p in pts:
            y, x = divmod(p, w)
            toph[x] = min(toph.get(x, 10 ** 6), y)
        ltop = sum(toph[cols[i]] for i in range(n // 3)) / max(1, n // 3)
        rtop = sum(toph[cols[i]] for i in range(n - n // 3, n)) / max(1, n - n // 3)
        print("   comp%d px=%-5d span x%3d..%3d y%3d..%3d  bottomL%5.1f bottomR%5.1f (R-L%+5.1f)  topL%5.1f topR%5.1f"
              % (ci, len(pts), x0, x1, y0, y1, lh, rh, rh - lh, ltop, rtop))
    print()


print("### neutral/front-on risk: shield idle")
for c in (0, 9):
    feet_report("shield_v2.png", 0, c)
print("### shield walk (direction already agreed: right)")
for c in (0, 4):
    feet_report("shield_v2.png", 1, c)
print("### shield attack")
for c in range(10):
    feet_report("shield_v2.png", 2, c)
