"""Probe a client bundle: list object types and sprite/texture names.

Usage: python output/_tmp_new/probe_ui.py "ui/[uc]stage.ab" [...]
Reads the installed client only; never writes to it.
"""
import sys
from collections import Counter
from pathlib import Path

import UnityPy
from UnityPy.helpers import CompressionHelper

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from extract_client_ui import decode  # noqa: E402  (registers flag-4 decoder)

CompressionHelper.DECOMPRESSION_MAP[4] = decode

ROOT = next(Path("C:/Users/34844").glob(
    "*/Arknights bilibili/games/Arknights/Arknights_Data/StreamingAssets/AB/Windows"))


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    for relative in sys.argv[1:]:
        path = ROOT / relative
        print("=" * 8, relative, "exists:", path.is_file())
        if not path.is_file():
            continue
        env = UnityPy.load(str(path))
        kinds: Counter = Counter()
        rows: list[tuple[str, str, str]] = []
        for obj in env.objects:
            kind = obj.type.name
            kinds[kind] += 1
            if kind in ("Sprite", "Texture2D"):
                try:
                    item = obj.read()
                    size = getattr(item, "m_Rect", None)
                    dims = ""
                    if kind == "Texture2D":
                        dims = f"{item.m_Width}x{item.m_Height}"
                    elif size is not None:
                        dims = f"{int(size.width)}x{int(size.height)}"
                    rows.append((kind, item.m_Name, dims))
                except Exception as exc:  # noqa: BLE001
                    rows.append((kind, f"<error {str(exc)[:60]}>", ""))
        print("  types:", dict(kinds))
        for kind, name, dims in sorted(rows):
            print(f"    {kind:<9} {name}  {dims}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
