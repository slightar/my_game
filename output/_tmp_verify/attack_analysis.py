"""End-to-end: for a left-facing guard, where does the Attack clip actually land?

Instead of screenshotting (the GPU readback proved unreliable in this environment),
this reproduces DrawUnit's blit arithmetic exactly in Python and composites the real
sheet cell, then measures the silhouette's centroid against the unit's feet. That is
the same computation the renderer performs, with no window and no GL involved.
"""
import os
from PIL import Image

BASE = r"D:\my_game\assets\enemies\mobs"
CELL = 192
DEST = 512
# EnemyRenderer::DrawUnit: DrawTexturePro(tex, {frame*192, row*192, flip?-192:192, 192},
#   {feet.x - anchorX/192*spriteSize, feet.y - anchors.y/192*spriteSize, spriteSize, spriteSize}, ...)
SPRITE = 192.0          # EnemyData(Shield).spriteSize
ANCHOR = (96.0, 180.0)  # from manifest: anchor [96, 180]


def blit(sheet, row, frame, facing, declared, feet=(256, 400)):
    im = Image.open(os.path.join(BASE, sheet)).convert("RGBA")
    cell = im.crop((frame * CELL, row * CELL, frame * CELL + CELL, row * CELL + CELL))
    flip = facing != declared
    if flip:
        cell = cell.transpose(Image.FLIP_LEFT_RIGHT)
    anchorX = (192 - ANCHOR[0]) if flip else ANCHOR[0]
    # DrawTexturePro scales the 192px cell to SPRITE px; DEST canvas is 1:1 with screen px
    cell = cell.resize((int(SPRITE), int(SPRITE)), Image.NEAREST)
    out = Image.new("RGBA", (DEST, DEST), (0, 0, 0, 0))
    x = int(round(feet[0] - anchorX / 192.0 * SPRITE))
    y = int(round(feet[1] - ANCHOR[1] / 192.0 * SPRITE))
    out.alpha_composite(cell, (x, y))
    return out, feet


def lean(img, feet):
    a = img.getchannel("A").load()
    sx = n = 0
    minx, maxx = DEST, -1
    for y in range(DEST):
        for x in range(DEST):
            if a[x, y] > 60:
                sx += x
                n += 1
                minx = min(minx, x)
                maxx = max(maxx, x)
    if n == 0:
        return None, 0, 0, 0
    cx = sx / n
    return cx - feet[0], minx - feet[0], maxx - feet[0], n


print("Shield, spriteSize=192, anchor=(96,180), feet=(256,400)")
print("declared per-row facing = [-1, 1, -1, -1]  (manifest shield)")
print("attack row override     = [L L L L R R R R R L]  (kShieldAttackNativeRight)")
print()
ATTACK_NATIVE = [-1, -1, -1, -1, 1, 1, 1, 1, 1, -1]
for facing, tag in ((-1, "guard faces LEFT  (u.facing=-1)"), (1, "guard faces RIGHT (u.facing=+1)")):
    print("=== %s ===" % tag)
    print("  attack row (row 2):  centroid  xmin  xmax   px   flip")
    for f in range(10):
        declared = ATTACK_NATIVE[f]
        img, feet = blit("shield_v2.png", 2, f, facing, declared=declared)
        c, x0, x1, n = lean(img, feet)
        if c is None:
            print("    f%-2d  EMPTY" % f)
        else:
            print("    f%-2d   %+7.1f %+5d %+5d  %5d   %s"
                  % (f, c, x0, x1, n, "yes" if facing != declared else "no"))
    print("  idle row (row 0) for reference:")
    for f in (0,):
        img, feet = blit("shield_v2.png", 0, f, facing, declared=-1)
        c, x0, x1, n = lean(img, feet)
        print("    f%-2d   %+7.1f %+5d %+5d  %5d" % (f, c, x0, x1, n))
    print()

print("AttackArea() spans feet.x to feet.x-reach for facing=-1  (reach=85 for Shield)")
print("  -> strike frames must sit at NEGATIVE x, which is where collision resolves.")
