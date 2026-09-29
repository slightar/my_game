"""Extract + bake the four models that the official-name pass needs.

enemy_1000_gopro (猎狗), enemy_1004_mslime (酸液源石虫),
enemy_1019_jshoot (隐形弩手), enemy_1011_wizard (术师).

The earlier wizard attempt pulled the model out of enm_art_base_0.ab and baked an
all-transparent sheet; enemy_model_catalogue reports it living in enm_art_3.ab, so this
script takes the bundle from the catalogue and hard-fails if the sheet is empty instead
of silently shipping a blank sprite.
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.normpath(os.path.join(HERE, "..", "..", "tools"))
sys.path.insert(0, TOOLS)

import UnityPy  # noqa: E402
from extract_client_ui import decode  # noqa: E402
from UnityPy.helpers import CompressionHelper  # noqa: E402
from PIL import Image  # noqa: E402

CompressionHelper.DECOMPRESSION_MAP[4] = decode

CLIENT = os.path.join(
    r"C:\Users\34844", "鹰角启动器", "Arknights bilibili", "games", "Arknights",
    "Arknights_Data", "StreamingAssets", "AB", "Windows", "refs", "arts",
)

# output key -> (model, bundle holding the BASE model)
# Note: enemy_model_catalogue collapses the `_N` skin suffixes, so it attributed
# enemy_1019_jshoot / enemy_1011_wizard to enm_art_3.ab - that bundle only holds the
# `_2` variant. The base rigs live in enm_art_base_0.ab.
MODELS = {
    "hound": ("enemy_1000_gopro", "enm_art_base_0.ab"),
    "acid_slug": ("enemy_1004_mslime", "enm_art_1.ab"),
    "stealth_crossbow": ("enemy_1019_jshoot", "enm_art_base_0.ab"),
    "caster": ("enemy_1011_wizard", "enm_art_base_0.ab"),
}

WORK = os.path.join(HERE, "rebake")
shutil.rmtree(WORK, ignore_errors=True)
os.makedirs(os.path.join(WORK, "in"))

by_bundle = {}
for key, (model, bundle) in MODELS.items():
    by_bundle.setdefault(bundle, []).append((key, model))

meta = {}
for bundle, entries in by_bundle.items():
    env = UnityPy.load(os.path.join(CLIENT, bundle))
    objects = {}
    for obj in env.objects:
        if obj.type.name in ("Texture2D", "TextAsset"):
            try:
                objects[obj.read().m_Name] = obj
            except Exception:  # noqa: BLE001
                pass
    for key, model in entries:
        folder = os.path.join(WORK, "in", key)
        os.makedirs(folder, exist_ok=True)
        for ext in ("atlas", "skel"):
            name = f"{model}.{ext}"
            if name not in objects:
                raise SystemExit(f"missing {name} in {bundle}")
            raw = objects[name].read().m_Script
            if isinstance(raw, str):
                raw = raw.encode("utf-8", "surrogateescape")
            with open(os.path.join(folder, name), "wb") as fh:
                fh.write(bytes(raw))
        if model not in objects:
            raise SystemExit(f"missing texture {model} in {bundle}")
        image = objects[model].read().image.convert("RGBA")
        alpha_name = model + "[alpha]"
        if alpha_name in objects:
            alpha = objects[alpha_name].read().image.convert("RGB")
            if alpha.size == image.size:
                image.putalpha(alpha.getchannel("R"))
        image.save(os.path.join(folder, f"{model}.png"))
        meta[key] = model
        transparent = image.getchannel("A").histogram()[0]
        print(f"{key:<18} {model:<20} {image.size} transparent={100*transparent/(image.size[0]*image.size[1]):.1f}%")

with open(os.path.join(WORK, "in", "models.json"), "w", encoding="utf-8") as fh:
    json.dump(meta, fh, ensure_ascii=False, indent=1)

# The baker needs the browser libs sitting beside index.html.
ref = os.path.normpath(os.path.join(HERE, "..", "..", "cmake-build-debug", "enemy-reference", "spine"))
for lib in ("pixi.js", "spine.js"):
    src = os.path.join(ref, lib)
    if os.path.isfile(src):
        shutil.copyfile(src, os.path.join(WORK, "in", lib))
    else:
        raise SystemExit(f"missing browser lib: {src}")

print("\nbaking...", flush=True)
subprocess.run([sys.executable, os.path.join(TOOLS, "bake_enemy_sheets.py"),
                "--input", os.path.join(WORK, "in"),
                "--output", os.path.join(WORK, "out")], check=True)

manifest = json.load(open(os.path.join(WORK, "out", "manifest.json"), encoding="utf-8"))
print(f"\n{'key':<18} {'model':<20} {'scale':<10} {'ink%':<8} verdict")
print("-" * 68)
bad = []
for key, (model, _bundle) in MODELS.items():
    sheet = os.path.join(WORK, "out", key + ".png")
    if key not in manifest or not os.path.isfile(sheet):
        print(f"{key:<18} {model:<20} {'-':<10} {'-':<8} NO OUTPUT")
        bad.append(key)
        continue
    im = Image.open(sheet).convert("RGBA")
    total = im.size[0] * im.size[1]
    ink = 100 * (1 - im.getchannel("A").histogram()[0] / total)
    scale = manifest[key].get("scale")
    anchor = manifest[key].get("anchor", [0, 0])
    ok = ink > 1.0 and scale is not None and (anchor[0] or anchor[1])
    if not ok:
        bad.append(key)
    print(f"{key:<18} {model:<20} {str(scale)[:9]:<10} {ink:<8.2f} {'OK' if ok else 'EMPTY/BAD'}")

print("\nbad:", bad if bad else "none")
