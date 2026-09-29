"""Verify the enemy roster capture numerically.

Image reads are not available in-session ("model does not support images"), so a visual
"looks right" check is impossible. Instead crop each card region out of enemy-roster.png
and measure ink coverage against the card fill colour: a blank or misplaced sprite shows
up as ~0%, and a card drawn outside its slot is caught by the neighbouring coverages.

Also reports the per-card ink *height* so a collapsed / fully transparent bake is visible.
"""
import pathlib

from PIL import Image

QA = pathlib.Path(r"D:\my_game\cmake-build-debug\character-qa")
CARD_FILL = (221, 227, 226)

# Must mirror enemy_render_checks.h.
SHEET = QA / "enemy-roster.png"
PITCH, HALF_W, CARD_H = 213, 95, 288
TOP0, TOP1 = 100, 396
COLS = 6


def ink_fraction(im: Image.Image, box) -> tuple[float, int]:
    cell = im.crop(box).convert("RGB")
    px = cell.load()
    w, h = cell.size
    hits = rows = 0
    for y in range(h):
        row_hit = False
        for x in range(w):
            r, g, b = px[x, y]
            if abs(r - CARD_FILL[0]) + abs(g - CARD_FILL[1]) + abs(b - CARD_FILL[2]) > 24:
                hits += 1
                row_hit = True
        if row_hit:
            rows += 1
    return hits / (w * h), rows


def main() -> None:
    im = Image.open(SHEET)
    print(f"{SHEET.name} {im.size}")
    kinds = 11
    for i in range(kinds):
        x = 106 + (i % COLS) * PITCH
        top = TOP0 if i < COLS else TOP1
        box = (int(x - HALF_W), top, int(x + HALF_W), top + CARD_H)
        frac, rows = ink_fraction(im, box)
        flag = "OK" if frac > 0.01 else "EMPTY"
        print(f"card {i:>2} box={box} ink={frac * 100:6.2f}% inkRows={rows:>3} {flag}")

    print()
    for name in ("enemy-directions.png", "enemy-telegraphs.png", "enemy-stealth.png"):
        p = QA / name
        if not p.is_file():
            print(name, "MISSING")
            continue
        img = Image.open(p).convert("RGB")
        total = img.size[0] * img.size[1]
        px = img.load()
        hits = sum(
            1
            for y in range(img.size[1])
            for x in range(img.size[0])
            if abs(px[x, y][0] - 188) + abs(px[x, y][1] - 198) + abs(px[x, y][2] - 203) > 24
        )
        print(f"{name}: {100 * hits / total:.2f}% non-background")


if __name__ == "__main__":
    main()
