"""List and extract enemy models from a locally installed Arknights client.

The client bundles under refs/arts use compression flag 4 (LZHAM). Unity removed the
LZHAM decoder from its runtime, so UnityPy cannot open them on its own; extract_client_ui
registers a working flag-4 decoder and this module reuses it.

Read-only with respect to the client installation: it only reads .ab bundles.

Usage:
    # list every enemy model (optionally filtered)
    python tools/enemy_model_catalogue.py list
    python tools/enemy_model_catalogue.py list --filter 1010

    # extract one model into a folder
    python tools/enemy_model_catalogue.py get enemy_1006_shield --output out/shield
"""
import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import UnityPy  # noqa: E402
from extract_client_ui import decode  # noqa: E402
from UnityPy.helpers import CompressionHelper  # noqa: E402

# Register the flag-4 (LZHAM) decoder. Reference:
# https://spottori.com/blog/for-kaltsit-i-reversed-arknights-client/
CompressionHelper.DECOMPRESSION_MAP[4] = decode

# The installer path is stable on Windows; override with --client when it moves.
DEFAULT_CLIENT = Path(
    r"C:\Users\34844\鹰角启动器\Arknights bilibili\games\Arknights"
    r"\Arknights_Data\StreamingAssets\AB\Windows"
)


def bundles(client: Path):
    root = client / "refs" / "arts"
    return sorted(f for f in os.listdir(root) if f.startswith("enm_art_") and f.endswith(".ab"))


def scan(client: Path):
    """Return {model_base: {'bundle':..., 'tex':[...], 'variants':[...]}}."""
    models = {}
    for bundle in bundles(client):
        env = UnityPy.load(str(client / "refs" / "arts" / bundle))
        for obj in env.objects:
            if obj.type.name not in ("Texture2D", "TextAsset"):
                continue
            try:
                name = obj.read().m_Name
            except Exception:  # noqa: BLE001
                continue
            if not name.startswith("enemy_"):
                continue

            kind, body = "tex", name
            for ext in (".atlas", ".skel"):
                if body.endswith(ext):
                    body, kind = body[: -len(ext)], ext.lstrip(".")
                    break
            is_alpha = body.endswith("[alpha]")
            if is_alpha:
                body = body[: -len("[alpha]")]

            parts = body.split("_")
            variant = ""
            if len(parts) >= 4 and parts[-1].isdigit():
                variant, body = "_" + parts[-1], "_".join(parts[:-1])

            rec = models.setdefault(body, {"bundle": bundle, "tex": 0, "variants": set(), "atlas": False, "skel": False})
            rec["variants"].add(variant or "_1")
            if kind == "atlas":
                rec["atlas"] = True
            elif kind == "skel":
                rec["skel"] = True
            elif not is_alpha:
                rec["tex"] += 1
    return models


def usable(models):
    return {k: v for k, v in models.items() if v["atlas"] and v["skel"] and v["tex"]}


def cmd_list(args):
    models = usable(scan(Path(args.client)))
    keys = sorted(models)
    if args.filter:
        keys = [k for k in keys if args.filter in k]
    for k in keys:
        rec = models[k]
        print(f"{k:<28} {rec['bundle']:<18} tex={rec['tex']} variants={','.join(sorted(rec['variants']))}")
    print(f"\n{len(keys)} model(s)")


def cmd_get(args):
    client = Path(args.client)
    target = args.model
    folder = Path(args.output) if args.output else Path(target)
    folder.mkdir(parents=True, exist_ok=True)

    models = usable(scan(client))
    if target not in models:
        # allow a prefix, e.g. "1006" -> enemy_1006_shield
        hits = [k for k in models if target in k]
        if len(hits) != 1:
            print(f"'{target}' matched {len(hits)} model(s): {hits[:10]}", file=sys.stderr)
            return 1
        target = hits[0]
    bundle = models[target]["bundle"]

    env = UnityPy.load(str(client / "refs" / "arts" / bundle))
    objects = {}
    for obj in env.objects:
        if obj.type.name in ("Texture2D", "TextAsset"):
            try:
                objects[obj.read().m_Name] = obj
            except Exception:  # noqa: BLE001
                pass

    written = 0
    for name, obj in objects.items():
        if not name.startswith(target):
            continue
        if obj.type.name == "TextAsset":
            if name.rsplit(".", 1)[-1] not in ("atlas", "skel"):
                continue
            raw = obj.read().m_Script
            if isinstance(raw, str):
                raw = raw.encode("utf-8", "surrogateescape")
            (folder / name).write_bytes(bytes(raw))
            print("wrote", folder / name)
            written += 1
        else:
            if name.endswith("[alpha]"):
                continue
            image = obj.read().image.convert("RGBA")
            alpha_obj = objects.get(name + "[alpha]")
            if alpha_obj:
                # The [alpha] texture keeps transparency in its RED channel.
                alpha_img = alpha_obj.read().image.convert("RGB")
                if alpha_img.size == image.size:
                    image.putalpha(alpha_img.getchannel("R"))
            path = folder / f"{name}.png"
            image.save(path)
            print("wrote", path, image.size)
            written += 1

    if not written:
        print(f"nothing extracted for {target}", file=sys.stderr)
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--client", default=str(DEFAULT_CLIENT), help="path to StreamingAssets/AB/Windows")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_list = sub.add_parser("list", help="list every enemy model in the client")
    p_list.add_argument("--filter", default="", help="substring filter, e.g. 1006")
    p_list.set_defaults(func=cmd_list)

    p_get = sub.add_parser("get", help="extract one model (atlas + skel + png)")
    p_get.add_argument("model", help="model base name or unique substring, e.g. enemy_1006_shield")
    p_get.add_argument("--output", default="", help="output folder (default: ./<model>)")
    p_get.set_defaults(func=cmd_get)

    args = parser.parse_args()
    if not Path(args.client).is_dir():
        print(f"client not found: {args.client}", file=sys.stderr)
        print("pass --client <...>/StreamingAssets/AB/Windows", file=sys.stderr)
        return 1
    return args.func(args) or 0


if __name__ == "__main__":
    sys.exit(main())
