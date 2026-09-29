"""Compare the source assets against the copy that sits next to the built executables."""
import hashlib
import os

PAIRS = [
    (r"D:\my_game\assets\enemies\mobs", r"D:\my_game\cmake-build-debug\assets\enemies\mobs"),
]

def digest(path):
    if not os.path.isfile(path):
        return None
    with open(path, "rb") as handle:
        return hashlib.sha1(handle.read()).hexdigest()[:12]

for source_dir, build_dir in PAIRS:
    print(f"source: {source_dir}")
    print(f"build : {build_dir}")
    print(f"{'file':22s} {'source':>14s} {'build':>14s}  state")
    names = sorted(set(os.listdir(source_dir)) | set(os.listdir(build_dir)))
    for name in names:
        if os.path.isdir(os.path.join(source_dir, name)):
            continue
        a = digest(os.path.join(source_dir, name))
        b = digest(os.path.join(build_dir, name))
        if a is None:
            state = "only in build"
        elif b is None:
            state = "MISSING in build"
        elif a == b:
            state = "same"
        else:
            state = "*** DIFFERENT ***"
        print(f"{name:22s} {str(a):>14s} {str(b):>14s}  {state}")

# What does the build dir's manifest say the shield file is?
import json

for label, path in (
    ("source", r"D:\my_game\assets\enemies\mobs\manifest.json"),
    ("build", r"D:\my_game\cmake-build-debug\assets\enemies\mobs\manifest.json"),
):
    if not os.path.isfile(path):
        print(f"\n{label} manifest: absent")
        continue
    with open(path, encoding="utf-8") as handle:
        data = json.load(handle)
    shield = data.get("shield", {})
    print(f"\n{label} manifest shield: file={shield.get('file')} facing={shield.get('facing')} "
          f"anchor={shield.get('anchor')}")
