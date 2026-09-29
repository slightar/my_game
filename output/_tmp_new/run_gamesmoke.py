"""Run game_enemy_smoke.exe in a scratch directory.

Game reads and writes save/settings next to the executable, so running the integration
smoke in place would overwrite the real cmake-build-debug progress file. This mirrors the
"copy the build directory" instruction the smoke's own header gives.
"""
import shutil
import subprocess
import sys
from pathlib import Path

BUILD = Path(r"D:\my_game\cmake-build-debug")
SCRATCH = Path(r"D:\my_game\output\_tmp_new\gamesmoke")


def main() -> int:
    exe = BUILD / "game_enemy_smoke.exe"
    if not exe.is_file():
        print("missing", exe, "- run build.py gamesmoke first", file=sys.stderr)
        return 1
    shutil.rmtree(SCRATCH, ignore_errors=True)
    SCRATCH.mkdir(parents=True)
    shutil.copy2(exe, SCRATCH / exe.name)
    # The smoke needs the bundled asset tree (characters + enemy sheets) beside the exe.
    shutil.copytree(BUILD / "assets", SCRATCH / "assets")
    result = subprocess.run([str(SCRATCH / exe.name)], cwd=SCRATCH,
                            capture_output=True, text=True)
    lines = [
        line for line in (result.stdout + result.stderr).splitlines()
        if not line.startswith(("INFO: TEXTURE", "INFO: SHADER", "INFO: FBO",
                                "INFO: FILEIO", "INFO: IMAGE", "INFO: AUDIO",
                                "INFO: AUTOMATION"))
    ]
    print("\n".join(lines))
    print("rc =", result.returncode, "| captures in", SCRATCH / "enemy-game-qa")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
