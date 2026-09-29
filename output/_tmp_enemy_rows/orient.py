from PIL import Image, ImageChops, ImageStat

ROOT = r"D:\my_game\assets\enemies\mobs"
img = Image.open(ROOT + r"\shield_v2.png").convert("RGBA")


def cell(row, col):
    return img.crop((col * 192, row * 192, col * 192 + 192, row * 192 + 192))


def diff(a, b):
    """Mean absolute alpha difference over the 192x192 cell."""
    return ImageStat.Stat(ImageChops.difference(a.getchannel("A"), b.getchannel("A"))).mean[0]


def mirror(a):
    return a.transpose(Image.FLIP_LEFT_RIGHT)


idle0 = cell(0, 0)
move0 = cell(1, 0)
print("--- how different is a pose from its own mirror (baseline) ---")
for label, c in [("idle f0", idle0), ("move f0", move0)]:
    print(f"{label} vs mirror: {diff(c, mirror(c)):.2f}")

print("\n--- idle f0  vs  candidate (lower = same orientation) ---")
for label, c in [("move f0", move0), ("move f0 mirrored", mirror(move0))]:
    print(f"idle f0 vs {label}: {diff(idle0, c):.2f}")

print("\n--- attack row: frame0 vs frame9, and each frame vs idle f0 / mirrored idle f0 ---")
a0, a9 = cell(2, 0), cell(2, 9)
print(f"atk f0 vs atk f9          : {diff(a0, a9):.2f}")
print(f"atk f9 vs idle f0         : {diff(a9, idle0):.2f}")
print(f"atk f9 vs mirror(idle f0) : {diff(a9, mirror(idle0)):.2f}")
print()
for c in range(10):
    f = cell(2, c)
    print(f"atk f{c}: vs idle f0 {diff(f, idle0):7.2f} | vs mirror(idle f0) {diff(f, mirror(idle0)):7.2f} | "
          f"vs move f0 {diff(f, move0):7.2f} | vs mirror(move f0) {diff(f, mirror(move0)):7.2f}")
