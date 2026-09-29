"""Copy assets/enemies/mobs -> cmake-build-debug/assets/enemies/mobs.

The offline g++ driver does not run CMake's POST_BUILD asset copy, so a freshly baked
sheet stays invisible at runtime until it is mirrored here. Same trap as the README warns
about; this script is the one-liner that closes it.
"""
import hashlib
import pathlib
import shutil

SRC = pathlib.Path(r"D:\my_game\assets\enemies\mobs")
DST = pathlib.Path(r"D:\my_game\cmake-build-debug\assets\enemies\mobs")


def digest(p: pathlib.Path) -> str | None:
    return hashlib.sha1(p.read_bytes()).hexdigest() if p.is_file() else None


def main() -> None:
    DST.mkdir(parents=True, exist_ok=True)
    copied, removed = [], []
    for src in sorted(SRC.iterdir()):
        if not src.is_file():
            continue
        dst = DST / src.name
        if digest(src) != digest(dst):
            shutil.copy2(src, dst)
            copied.append(src.name)
    source_names = {p.name for p in SRC.iterdir() if p.is_file()}
    for dst in sorted(DST.iterdir()):
        if dst.is_file() and dst.name not in source_names:
            dst.unlink()
            removed.append(dst.name)
    print("copied :", ", ".join(copied) or "(nothing)")
    print("removed:", ", ".join(removed) or "(nothing)")


if __name__ == "__main__":
    main()
