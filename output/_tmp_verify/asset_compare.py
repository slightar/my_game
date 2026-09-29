"""Every asset copy the game could load, compared hash-for-hash, plus a per-frame
facing readout for every shield-like sheet on disk.

Rationale: the renderer loads from GetApplicationDirectory()/assets/... (the exe's own
folder), NOT from the source tree.  A stale or different copy there would mean the fix
is reasoned against a sheet the game never renders.  So we locate every manifest.json
and every shield*.png under the project, hash them, and measure the leg centroid of the
attack row for each.
"""
import hashlib
import json
from pathlib import Path
from PIL import Image

ROOT = Path(r"D:\my_game")
CELL = 192
BAND = (128, 186)


def sha1(p):
    h = hashlib.sha1()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()[:12]


def legs_bias(path, row=2, frames=range(10)):
    img = Image.open(path).convert("RGBA")
    out = {}
    for fr in frames:
        cell = img.crop((fr * CELL, row * CELL, fr * CELL + CELL, row * CELL + CELL))
        a = cell.getchannel("A")
        px = a.load()
        tot = 0
        wsum = 0
        for y in range(*BAND):
            for x in range(CELL):
                v = px[x, y]
                if v < 24:
                    continue
                tot += v
                wsum += x * v
        out[fr] = None if tot == 0 else round(wsum / tot - CELL / 2, 2)
    return img.size, out


print("=== manifests ===")
for p in sorted(ROOT.rglob("manifest.json")):
    if any(seg in str(p).lower() for seg in ("_deps", "third_party", "node_modules")):
        continue
    try:
        data = json.loads(p.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"{p}\n    unreadable: {e}")
        continue
    sh = data.get("shield")
    print(f"{p}   sha1={sha1(p)}")
    if sh is None:
        print("    (no shield entry)")
    else:
        print(f"    shield.file={sh.get('file')!r}  facing={sh.get('facing')}  durations={sh.get('durations')}")

print("\n=== shield sheets ===")
seen = {}
for p in sorted(ROOT.rglob("shield*.png")):
    if any(seg in str(p).lower() for seg in ("_deps", "third_party", "_tmp", "output")):
        continue
    sz, out = legs_bias(p)
    h = sha1(p)
    print(f"\n{p}")
    print(f"    size={sz}  sha1={h}  bytes={p.stat().st_size:,}")
    print(f"    row2 leg bias per frame: {out}")
    seen.setdefault(h, []).append(str(p))

print("\n=== duplicate groups (identical content) ===")
for h, paths in seen.items():
    if len(paths) > 1:
        print(f"  {h}: {len(paths)} copies")
        for q in paths:
            print(f"      {q}")
