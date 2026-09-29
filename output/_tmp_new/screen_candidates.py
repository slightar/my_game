"""Check candidate models for the four animations the baker needs.

The bake tool maps Idle / Move(_Loop) / Attack / Die by name. A model missing any of
those would fail at bake time, so screen candidates here first.
"""
import os
import re
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

CANDIDATES = {
    # hounds / beasts
    "hound_a": "enemy_1030_wteeth",
    "hound_b": "enemy_1383_windog",
    "hound_c": "enemy_10107_mjcdog",
    "hound_d": "enemy_10129_pycdog",
    "hound_e": "enemy_8011_mcndog",
    # originium slugs (variants)
    "slug_b": "enemy_1004_mslime",
    "slug_c": "enemy_1228_dslime",
    "slug_d": "enemy_1246_aslime",
    "slug_e": "enemy_1352_eslime",
    "slug_f": "enemy_1067_snslime",
    "slug_g": "enemy_1050_lslime",
    # casters
    "caster_a": "enemy_1011_wizard",
    "caster_b": "enemy_1022_dmage",
    "caster_c": "enemy_1023_jmage",
    "caster_d": "enemy_1068_snmage",
    "caster_e": "enemy_1128_spmage",
    "caster_f": "enemy_1225_dkmage",
    "caster_g": "enemy_1241_ltmage",
    "caster_h": "enemy_1168_dumage",
    # crossbows / stealth
    "xbow_b": "enemy_1012_dcross",
    "ghost_a": "enemy_1008_ghost",
    "ghost_b": "enemy_1026_aghost",
    "ghost_c": "enemy_1354_eghost",
    "lurker": "enemy_1009_lurker",
}

NEED = {"idle": ("idle",), "move": ("move", "move_loop"), "attack": ("attack",), "die": ("die", "dead", "death")}


def anim_names(skel_bytes):
    """Spine 3.8 .skel stores animation names as length-prefixed utf-8 strings.

    Rather than parse the binary format, scan for printable runs that look like
    animation names - that is enough to screen candidates.
    """
    found = set()
    for m in re.finditer(rb"[A-Za-z_][A-Za-z0-9_]{2,31}", skel_bytes):
        found.add(m.group().decode("ascii", "ignore"))
    return found


# group candidates by bundle so each bundle is opened once
by_bundle = {}
for key, model in CANDIDATES.items():
    by_bundle.setdefault(model, None)

env_cache = {}
results = {}

for key, model in CANDIDATES.items():
    if model not in env_cache:
        # find the bundle from the catalogue tool logic
        env_cache[model] = None

# locate bundles
model_bundle = {}
for bundle in sorted(f for f in os.listdir(CLIENT) if f.startswith("enm_art_") and f.endswith(".ab")):
    env = UnityPy.load(os.path.join(CLIENT, bundle))
    names = set()
    for obj in env.objects:
        try:
            names.add(obj.read().m_Name)
        except Exception:
            continue
    for model in CANDIDATES.values():
        if f"{model}.skel" in names:
            model_bundle[model] = (bundle, names)
    if len(model_bundle) == len(set(CANDIDATES.values())):
        break

print(f"located {len(model_bundle)}/{len(set(CANDIDATES.values()))} models\n")
print(f"{'key':<10} {'model':<22} {'idle':<5} {'move':<5} {'atk':<5} {'die':<5} bundle")
print("-" * 84)

ok = []
for key, model in CANDIDATES.items():
    if model not in model_bundle:
        print(f"{key:<10} {model:<22} NOT FOUND")
        continue
    bundle, names = model_bundle[model]
    # grab the skel bytes to scan animation names
    env = UnityPy.load(os.path.join(CLIENT, bundle))
    skel = None
    for obj in env.objects:
        if obj.type.name == "TextAsset":
            d = obj.read()
            if d.m_Name == f"{model}.skel":
                skel = d.m_Script
                if isinstance(skel, str):
                    skel = skel.encode("utf-8", "surrogateescape")
                break
    anims = anim_names(bytes(skel)) if skel else set()
    flags = {}
    for want, alts in NEED.items():
        flags[want] = any(a in alts for a in (x.lower() for x in anims))
    mark = lambda b: "Y" if b else "."
    line = (f"{key:<10} {model:<22} {mark(flags['idle']):<5} {mark(flags['move']):<5} "
            f"{mark(flags['attack']):<5} {mark(flags['die']):<5} {bundle}")
    print(line)
    if all(flags.values()):
        ok.append((key, model, bundle))

print()
print("fully animatable:", len(ok))
for key, model, bundle in ok:
    print("  ", key, model, bundle)
