"""Dump the RectTransform hierarchy of a client bundle as an indented tree.

The point is to recover the *real* pixel layout of a screen (positions, sizes,
anchors) instead of eyeballing it, so the HTML rebuild can be pixel-accurate.

Usage: python output/_tmp_new/dump_ui_tree.py "ui/[uc]stage.ab" > tree.txt
Reads the installed client only; never writes to it.
"""
import sys
from pathlib import Path

import UnityPy
from UnityPy.helpers import CompressionHelper

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from extract_client_ui import decode  # noqa: E402  (registers flag-4 decoder)

CompressionHelper.DECOMPRESSION_MAP[4] = decode

ROOT = next(Path("C:/Users/34844").glob(
    "*/Arknights bilibili/games/Arknights/Arknights_Data/StreamingAssets/AB/Windows"))


def fmt_vec(v, digits=1):
    if v is None:
        return "?"
    x = getattr(v, "x", v[0] if hasattr(v, "__getitem__") else v)
    y = getattr(v, "y", v[1] if hasattr(v, "__getitem__") else v)
    return f"({round(x, digits)}, {round(y, digits)})"


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = ROOT / sys.argv[1]
    env = UnityPy.load(str(path))

    rects: dict[int, object] = {}
    names: dict[int, str] = {}
    for obj in env.objects:
        if obj.type.name == "GameObject":
            try:
                g = obj.read()
                names[obj.path_id] = g.m_Name
            except Exception:  # noqa: BLE001
                names[obj.path_id] = f"<go {obj.path_id}>"
        elif obj.type.name == "RectTransform":
            try:
                rects[obj.path_id] = obj.read()
            except Exception:  # noqa: BLE001
                pass

    go_to_rect: dict[int, int] = {}
    for rid, r in rects.items():
        try:
            go_to_rect[r.m_GameObject.path_id] = rid
        except Exception:  # noqa: BLE001
            pass

    children: dict[int, list[int]] = {}
    roots: list[int] = []
    for rid, r in rects.items():
        try:
            father = r.m_Father.path_id
        except Exception:  # noqa: BLE001
            father = 0
        if father in rects:
            children.setdefault(father, []).append(rid)
        else:
            roots.append(rid)

    print(f"# {sys.argv[1]}  rects={len(rects)} gameobjects={len(names)} roots={len(roots)}")

    def walk(rid: int, depth: int, seen: set[int]) -> None:
        if rid in seen or depth > 12:
            return
        seen.add(rid)
        r = rects[rid]
        try:
            go = r.m_GameObject.path_id
        except Exception:  # noqa: BLE001
            go = 0
        name = names.get(go, f"<{go}>")
        try:
            apos = fmt_vec(r.m_AnchoredPosition)
            size = fmt_vec(r.m_SizeDelta)
            amin = fmt_vec(r.m_AnchorMin, 2)
            amax = fmt_vec(r.m_AnchorMax, 2)
            piv = fmt_vec(r.m_Pivot, 2)
            scale = fmt_vec(r.m_LocalScale, 3)
            active = "off" if not getattr(r, "m_IsActive", True) else "on"
        except Exception as exc:  # noqa: BLE001
            print("  " * depth + f"{name} <err {str(exc)[:60]}>")
            return
        print("  " * depth
              + f"{name}  pos={apos} size={size} anchor={amin}..{amax} pivot={piv}"
              + (f" scale={scale}" if scale != "(1.0, 1.0, 1.0)" else "")
              + (f" [{active}]" if active == "off" else ""))
        for child in sorted(children.get(rid, [])):
            walk(child, depth + 1, seen)

    for root in sorted(roots):
        walk(root, 0, set())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
