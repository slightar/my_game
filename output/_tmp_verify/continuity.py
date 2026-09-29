"""Settle the shield attack row's native facing by ANIMATION CONTINUITY, not by a
centroid heuristic that a lunge pose could fool.

Idea: an exported animation is temporally coherent.  Consecutive frames of a swing
should have high silhouette overlap (IoU) with their neighbour.  If frames 4-7 are
natively drawn facing the SAME way as the wind-up, then feeding them as-is keeps the
overlap high.  If they are natively drawn facing the OPPOSITE way (an export slip),
then mirroring exactly those frames is what restores coherence.

We score three interpretations of row 2:
  A  all native          (bash assumed same native facing as windup)
  B  mirror frames 4-7   (bash assumed natively opposite)
  C  mirror frames 4-9   (bash + recovery natively opposite)
and report the total IoU across the 0->1->...->9 chain.  The winner is the one the
artist actually drew.
"""
from PIL import Image

SHEET = r"D:\my_game\assets\enemies\mobs\shield_v2.png"
CELL = 192
ROW = 2
N = 10

img = Image.open(SHEET).convert("RGBA")


def mask(frame, mirror=False):
    cell = img.crop((frame * CELL, ROW * CELL, frame * CELL + CELL, ROW * CELL + CELL))
    if mirror:
        cell = cell.transpose(Image.FLIP_LEFT_RIGHT)
    a = cell.getchannel("A")
    return [1 if v >= 24 else 0 for v in a.getdata()]


def iou(m1, m2):
    inter = 0
    union = 0
    for a, b in zip(m1, m2):
        if a or b:
            union += 1
            if a and b:
                inter += 1
    return inter / union if union else 0.0


CANDS = {
    "A  all native": {f: False for f in range(N)},
    "B  mirror 4-7": {f: (4 <= f <= 7) for f in range(N)},
    "C  mirror 4-9": {f: (4 <= f <= 9) for f in range(N)},
}

for name, spec in CANDS.items():
    masks = [mask(f, spec[f]) for f in range(N)]
    pairs = []
    total = 0.0
    for f in range(N - 1):
        v = iou(masks[f], masks[f + 1])
        total += v
        pairs.append((f, f + 1, v))
    print(f"\n=== {name} ===  total IoU = {total:.3f}  (mean {total/(N-1):.3f})")
    line = "  ".join(f"{a}->{b}:{v:.3f}" for a, b, v in pairs)
    print("  " + line)
    worst = sorted(pairs, key=lambda p: p[2])[:3]
    print("  weakest joints: " + ", ".join(f"{a}->{b} {v:.3f}" for a, b, v in worst))

print("\nThe interpretation with the highest total IoU is the one the artist drew:")
print("animations are continuous, so the truthful reading maximises neighbour overlap.")
