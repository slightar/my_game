"""Render the ORIGINAL shield Spine skeleton headlessly and measure its direction.

The baked sheet can only tell us about silhouettes; the .skel/.atlas is the model
author's own data. Rendering the setup pose and every clip frame from the skeleton
gives the authored facing without any bake-script guesswork.
"""
import base64
import http.server
import json
import os
import socketserver
import subprocess
import tempfile
import threading

ROOT = r"D:\my_game\output\_tmp_verify\raw_render"
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
PORT = 8731
store = {"frames": None, "error": None}


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=ROOT, **kw)

    def log_message(self, *a):
        pass

    def do_POST(self):
        n = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(n)
        if self.path == "/frames":
            store["frames"] = json.loads(body)
        elif self.path == "/error":
            store["error"] = body.decode("utf-8", "replace")
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b"ok")


srv = socketserver.TCPServer(("127.0.0.1", PORT), Handler)
threading.Thread(target=srv.serve_forever, daemon=True).start()

profile = tempfile.mkdtemp(prefix="edgeprof_")
cmd = [
    EDGE, "--headless=new", "--disable-gpu", "--no-first-run", "--no-default-browser-check",
    "--user-data-dir=" + profile, "--virtual-time-budget=40000", "--dump-dom",
    "http://127.0.0.1:%d/index.html" % PORT,
]
try:
    subprocess.run(cmd, timeout=120, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
except subprocess.TimeoutExpired:
    print("edge timed out (frames may still have arrived)")

if store["error"]:
    print("PAGE ERROR:", store["error"][:2000])
frames = store["frames"]
if not frames:
    print("no frames were posted")
    raise SystemExit(1)

outdir = os.path.join(ROOT, "png")
os.makedirs(outdir, exist_ok=True)
for key, data in frames.items():
    with open(os.path.join(outdir, key + ".png"), "wb") as f:
        f.write(base64.b64decode(data))
print("wrote", len(frames), "frames to", outdir)
print("keys:", sorted(frames.keys())[:8], "...")
srv.shutdown()
