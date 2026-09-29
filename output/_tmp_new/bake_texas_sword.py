"""Bake the falling-sword sprite for 剑雨 out of the official char_102_texas atlas.

Source: .battle-staging/texas_atlas.png (== official `char_102_texas.png`, 512x512)
Region: `F_Weapon`  ->  xy: 108,468  size: 108,42  (from .battle-staging/texas.atlas)

The blade lies diagonally inside that box (dark hilt at lower-left, tip at upper-right).
We measure the ink's principal axis, rotate it to vertical, then flip it so the tip
points DOWN - the way Texas's blades fall into the ground in game. The result is trimmed
to its ink so the runtime draw is a plain unrotated blit.
"""
import io, math, os
from PIL import Image, ImageOps

ROOT = r"D:\my_game"
ATLAS = os.path.join(ROOT, r".battle-staging\texas_atlas.png")
OUT = os.path.join(ROOT, r"assets\operators\texas_sword.png")

# F_Weapon region straight from texas.atlas
X, Y, W, H = 108, 468, 108, 42

atlas = Image.open(ATLAS).convert("RGBA")
box = atlas.crop((X, Y, X + W, Y + H))

# --- principal axis of the ink (alpha-weighted PCA) -------------------------------
px = box.load()
xs, ys, n = [], [], 0
for y in range(box.height):
    for x in range(box.width):
        a = px[x, y][3]
        if a > 40:
            xs.append(x); ys.append(y); n += 1
cx, cy = sum(xs) / n, sum(ys) / n
sxx = sum((x - cx) ** 2 for x in xs) / n
syy = sum((y - cy) ** 2 for y in ys) / n
sxy = sum((x - cx) * (y - cy) for x, y in zip(xs, ys)) / n
theta = 0.5 * math.atan2(2 * sxy, sxx - syy)         # radians, +ve = axis tilts down-right
angle = math.degrees(theta)
print(f"ink px={n}  centroid=({cx:.1f},{cy:.1f})  principal axis = {angle:+.2f} deg from +X")

# The blade runs lower-left -> upper-right, i.e. axis ~ -10 deg. We want her tip UP:
# rotate counter-clockwise by (90 + angle). PIL's rotate() is counter-clockwise, positive.
tip_up = box.rotate(90.0 + angle, resample=Image.BICUBIC, expand=True)
tip_up = ImageOps.flip(tip_up)                        # tip DOWN, the way blades fall

bb = tip_up.getchannel("A").getbbox()
sword = tip_up.crop(bb)
sword = ImageOps.expand(sword, border=2)              # 2px breathing room

os.makedirs(os.path.dirname(OUT), exist_ok=True)
sword.save(OUT)
print(f"wrote {OUT}: {sword.size[0]}x{sword.size[1]}")
print("alpha bbox of result:", sword.getchannel('A').getbbox())
