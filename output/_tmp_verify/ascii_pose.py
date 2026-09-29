"""Render sprite frames as ASCII density maps so the pose can be read as text.

An image Read comes back filtered for this model, so the silhouette is downsampled
and printed instead: each cell's opaque coverage becomes a character. This makes the
authored facing directly readable (which side the shield / crossbow / sword is on,
which way the toes point) instead of being inferred from a centroid, which also
reacts to the pose leaning back during a windup.
"""
import os
import sys
from PIL import Image, ImageOps

BASE = r"D:\my_game\assets\enemies\mobs"
CELL = 192
RAMP = " .:-=+*#%@"


def render(sheet, row, col, height=32, label=""):
    im = Image.open(os.path.join(BASE, sheet)).convert("RGBA")
    cell = im.crop((col * CELL, row * CELL, col * CELL + CELL, row * CELL + CELL))
    alpha = cell.getchannel("A")
    bbox = alpha.point(lambda v: 255 if v > 40 else 0).getbbox()
    if bbox is None:
        print("empty"); return
    pad = 2
    box = (max(0, bbox[0] - pad), max(0, bbox[1] - pad),
           min(CELL, bbox[2] + pad), min(CELL, bbox[3] + pad))
    crop = cell.crop(box)
    w, h = crop.size
    # a console cell is about twice as tall as wide, so double the column count
    tw = min(110, max(2, round(2 * height * w / h)))
    small = crop.resize((tw, height), Image.LANCZOS)
    # a 1px vertical rule at the cell centre keeps left/right honest
    cx = int(round((96 - box[0]) * tw / w))
    print("\n--- %s  row%d f%d  bbox=%s  (cell x=%d is the cell centre; '|' marks it)" %
          (sheet, row, col, bbox, cx))
    if label:
        print("    %s" % label)
    px = small.load()
    for y in range(height):
        line = []
        for x in range(tw):
            if x == cx:
                line.append("|")
                continue
            a = px[x, y][3] / 255.0
            line.append(RAMP[min(9, int(a * 9.999))])
        print("   " + "".join(line))
    # bottom 12 rows = feet band, to show which way the toes point
    print("    feet band (bottom 35%% of the figure):")
    fy = int(h * 0.65)
    feet = cell.crop((box[0], box[1] + fy, box[2], box[3]))
    fw, fh = feet.size
    ftw = min(110, max(2, round(2 * 10 * fw / fh)))
    smallf = feet.resize((ftw, 10), Image.LANCZOS)
    fcx = int(round((96 - box[0]) * ftw / fw))
    pxf = smallf.load()
    for y in range(10):
        line = []
        for x in range(ftw):
            line.append("|" if x == fcx else RAMP[min(9, int(pxf[x, y][3] / 255.0 * 9.999))])
        print("   " + "".join(line))


TARGETS = [
    ("shield_v2.png", 1, 0, "move f0 - manifest claims native RIGHT"),
    ("shield_v2.png", 0, 0, "idle f0 - manifest claims native LEFT"),
    ("shield_v2.png", 2, 3, "attack f3 end of windup - manifest claims native LEFT"),
    ("shield_v2.png", 2, 5, "attack f5 mid strike"),
    ("crossbow.png", 2, 5, "CONTROL crossbow attack f5 - weapon must point at the target"),
    ("soldier.png", 2, 5, "CONTROL soldier attack f5 - sword must point at the target"),
]
for i, (s, r, c, lab) in enumerate(TARGETS):
    render(s, r, c, height=34, label=lab)
