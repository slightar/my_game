"""Run the spine rig diagnostic in headless Edge and print the JSON report."""
import functools
import http.server
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "rebake", "in")          # extracted rigs + pixi.js/spine.js
DIAG = os.path.join(HERE, "diag")
WORK = os.path.join(HERE, "diag_in")

shutil.rmtree(WORK, ignore_errors=True)
os.makedirs(WORK)
for entry in ("caster", "stealth_crossbow", "hound"):
    shutil.copytree(os.path.join(SRC, entry), os.path.join(WORK, entry))
for lib in ("pixi.js", "spine.js"):
    shutil.copyfile(os.path.join(SRC, lib), os.path.join(WORK, lib))
models = {k: json.load(open(os.path.join(SRC, "models.json"), encoding="utf-8"))[k]
          for k in ("caster", "stealth_crossbow", "hound")}
with open(os.path.join(WORK, "models.json"), "w", encoding="utf-8") as fh:
    json.dump(models, fh, ensure_ascii=False, indent=1)
shutil.copyfile(os.path.join(DIAG, "index.html"), os.path.join(WORK, "index.html"))

result = {}
done = threading.Event()
errors = []


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        body = self.rfile.read(int(self.headers["Content-Length"]))
        if self.path == "/done":
            result["report"] = body.decode()
        elif self.path == "/error":
            errors.append(body.decode())
        done.set()
        self.send_response(200)
        self.end_headers()

    def log_message(self, *_):
        pass


server = http.server.ThreadingHTTPServer(("127.0.0.1", 0),
                                         functools.partial(Handler, directory=WORK))
threading.Thread(target=server.serve_forever, daemon=True).start()
with tempfile.TemporaryDirectory(prefix="diag-") as profile:
    proc = subprocess.Popen(
        [r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
         "--headless", "--disable-gpu", "--enable-unsafe-swiftshader", "--no-first-run",
         "--no-default-browser-check", "--disable-extensions", "--disable-background-networking",
         "--user-data-dir=" + profile, f"http://127.0.0.1:{server.server_port}/index.html"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        if not done.wait(120):
            raise SystemExit("diagnostic timed out")
    finally:
        subprocess.run(["taskkill", "/PID", str(proc.pid), "/T", "/F"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        proc.wait()
        server.shutdown()

if errors:
    print("PAGE ERROR:\n", errors[0])
    sys.exit(1)
print(result.get("report", "no report"))
