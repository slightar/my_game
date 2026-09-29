"""Bake every caster candidate and report which ones produce a non-empty sheet.

The wizard baked to an all-transparent sheet, so screen the alternatives by measuring
how much ink actually lands on the produced sheet rather than trusting the metadata.
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(HERE, "..", "..", "tools")
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

CANDIDATES = {
    "c_dmage": ("enemy_1022_dmage", "enm_art_3.ab"),
    "c_jmage": ("enemy_1023_jmage", "enm_art_base_0.ab"),
    "c_snmage": ("enemy_1068_snmage", "enm_art_4.ab"),
    "c_spmage": ("enemy_1128_spmage", "enm_art_6.ab"),
    "c_dkmage": ("enemy_1225_dkmage", "enm_art_10.ab"),
    "c_ltmage": ("enemy_1241_ltmage", "enm_art_10.ab"),
    "c_dumage": ("enemy_1168_dumage", "enm_art_8.ab"),
    "c_wizard": ("enemy_1011_wizard", "enm_art_base_0.ab"),
}

WORK = os.path.join(HERE, "caster_try")
shutil.rmtree(WORK, ignore_errors=True)
os.makedirs(WORK)

# extract all candidates into one input folder
by_bundle = {}
for key, (model, bundle) in CANDIDATES.items():
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
            raw = objects[f"{model}.{ext}"].read().m_Script
            if isinstance(raw, str):
                raw = raw.encode("utf-8", "surrogateescape")
            with open(os.path.join(folder, f"{model}.{ext}"), "wb") as fh:
                fh.write(bytes(raw))
        image = objects[model].read().image.convert("RGBA")
        alpha_name = model + "[alpha]"
        if alpha_name in objects:
            alpha = objects[alpha_name].read().image.convert("RGB")
            if alpha.size == image.size:
                image.putalpha(alpha.getchannel("R"))
        image.save(os.path.join(folder, f"{model}.png"))
        meta[key] = model

with open(os.path.join(WORK, "in", "models.json"), "w", encoding="utf-8") as fh:
    json.dump(meta, fh, ensure_ascii=False, indent=1)

PY = sys.executable
baker = os.path.join(TOOLS, "bake_enemy_sheets.py")
print("baking", len(meta), "candidates...", flush=True)
subprocess.run([PY, baker, "--input", os.path.join(WORK, "in"),
                "--output", os.path.join(WORK, "out")], check=True)

manifest_path = os.path.join(WORK, "out", "manifest.json")
m = json.load(open(manifest_path, encoding="utf-8"))
print(f"\n{'key':<10} {'model':<20} {'scale':<9} {'ink%':<7} verdict")
print("-" * 62)
good = []
for key in CANDIDATES:
    if key not in m:
        print(f"{key:<10} missing from manifest")
        continue
    p = os.path.join(WORK, "out", key + ".png")
    if not os.path.isfile(p):
        print(f"{key:<10} no sheet written")
        continue
    im = Image.open(p).convert("RGBA")
    a = im.getchannel("A").histogram()
    total = im.size[0] * im.size[1]
    ink = 100 * (1 - a[0] / total)
    scale = m[key].get("scale")
    anchor = m[key].get("anchor", [0, 0])
    ok = ink > 1.0 and scale is not None and (anchor[0] or anchor[1])
    if ok:
        good.append(key)
    print(f"{key:<10} {CANDIDATES[key][0]:<20} {str(scale)[:8]:<9} {ink:<7.2f} {'OK' if ok else 'EMPTY/BAD'}")

print("\nusable casters:", good)
