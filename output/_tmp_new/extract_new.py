"""Extract the four new mob models for the project, alpha merged.

Writes to output/_tmp_new/models/<key>/ so the existing bake tool can consume it
(models.json + one folder per key + pixi.js/spine.js).
"""
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools"))

import UnityPy  # noqa: E402
from extract_client_ui import decode  # noqa: E402
from UnityPy.helpers import CompressionHelper  # noqa: E402

CompressionHelper.DECOMPRESSION_MAP[4] = decode

CLIENT = os.path.join(
    r"C:\Users\34844", "鹰角启动器", "Arknights bilibili", "games", "Arknights",
    "Arknights_Data", "StreamingAssets", "AB", "Windows", "refs", "arts",
)
OUT = os.path.join(os.path.dirname(__file__), "models")

# project key -> (model name, bundle)
MODELS = {
    "hound": ("enemy_1030_wteeth", "enm_art_base_0.ab"),
    "caster": ("enemy_1068_snmage", "enm_art_4.ab"),
    "slug_high": ("enemy_1352_eslime", "enm_art_13.ab"),
    "stealth_crossbow": ("enemy_1012_dcross", "enm_art_3.ab"),
}

os.makedirs(OUT, exist_ok=True)

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
        folder = os.path.join(OUT, key)
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

        image = objects[model].read().image.convert("RGBA")
        alpha_name = model + "[alpha]"
        if alpha_name in objects:
            # Transparency lives in the RED channel of the [alpha] texture.
            alpha = objects[alpha_name].read().image.convert("RGB")
            if alpha.size == image.size:
                image.putalpha(alpha.getchannel("R"))
        image.save(os.path.join(folder, f"{model}.png"))
        meta[key] = model
        print(f"{key:<18} {model:<20} {image.size}")

with open(os.path.join(OUT, "models.json"), "w", encoding="utf-8") as fh:
    json.dump(meta, fh, ensure_ascii=False, indent=1)

# The baker needs the browser libs beside index.html.
ref = os.path.join(os.path.dirname(__file__), "..", "..", "cmake-build-debug", "enemy-reference", "spine")
for lib in ("pixi.js", "spine.js"):
    src = os.path.join(ref, lib)
    if os.path.isfile(src):
        shutil.copyfile(src, os.path.join(OUT, lib))
        print("copied", lib)
    else:
        print("WARNING: missing", src)

print("\nmodels.json:", json.dumps(meta, ensure_ascii=False))
