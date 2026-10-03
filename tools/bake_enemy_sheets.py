"""Bake previously extracted models using an isolated headless Edge instance.
Input folder needs models.json, model folders and developer-only pixi.js/spine.js.
"""
import argparse
import base64
import functools
import http.server
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--page', type=Path, default=Path(__file__).with_suffix('.html'))
    args = p.parse_args()
    args.input = args.input.resolve(); args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.page, args.input / 'index.html')
    models = json.loads((args.input / 'models.json').read_text())
    done = threading.Event(); errors = []
    class Handler(http.server.SimpleHTTPRequestHandler):
        def do_POST(self):
            body = self.rfile.read(int(self.headers['Content-Length']))
            if self.path == '/error': errors.append(body.decode()); done.set()
            elif self.path == '/done':
                (args.output / 'manifest.json').write_bytes(body); done.set()
            elif self.path.startswith('/bake/') and self.path[6:] in models:
                name = self.path[6:]
                (args.output / (name + '.png')).write_bytes(base64.b64decode(body))
                print('Baked', name, flush=True)
            else: self.send_error(400); return
            self.send_response(200); self.end_headers()
        def log_message(self, *_): pass
    server = http.server.ThreadingHTTPServer(('127.0.0.1',0),functools.partial(Handler,directory=str(args.input)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    with tempfile.TemporaryDirectory(prefix='enemy-bake-') as profile:
        process = subprocess.Popen([r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
            '--headless', '--disable-gpu', '--enable-unsafe-swiftshader', '--no-first-run',
            '--no-default-browser-check', '--disable-extensions', '--disable-background-networking',
            '--user-data-dir='+profile, f'http://127.0.0.1:{server.server_port}/index.html'],
            stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            if not done.wait(90): raise RuntimeError('Offline render timed out')
            if errors: raise RuntimeError(errors[0])
        finally:
            subprocess.run(['taskkill','/PID',str(process.pid),'/T','/F'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            process.wait();server.shutdown()

if __name__ == '__main__': main()
