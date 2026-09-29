"""List enemy mob sheets with size/mtime and hash the shield sheet."""
import hashlib
import os
import time

DIR = r"D:\my_game\assets\enemies\mobs"

for name in sorted(os.listdir(DIR)):
    path = os.path.join(DIR, name)
    if not os.path.isfile(path):
        continue
    stat = os.stat(path)
    stamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(stat.st_mtime))
    digest = ""
    if name.endswith(".png"):
        with open(path, "rb") as handle:
            digest = hashlib.sha1(handle.read()).hexdigest()[:12]
    print(f"{name:24s} {stat.st_size:9d}  {stamp}  {digest}")

sheet = os.path.join(DIR, "shield_v2.png")
if os.path.isfile(sheet):
    with open(sheet, "rb") as handle:
        print("\nshield_v2.png sha256:", hashlib.sha256(handle.read()).hexdigest())

print("\nany other shield-named sheets:")
for root, _dirs, files in os.walk(r"D:\my_game\assets"):
    for name in files:
        if "shield" in name.lower():
            print("  ", os.path.join(root, name))
