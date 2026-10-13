"""Download publicly available reference quality for local frame analysis.

No credentials, signed-in browser state, or quality restrictions are bypassed.
Reference videos stay in ignored output/, never in distributed game assets.
"""
import argparse
import json
import subprocess
import sys
import urllib.parse
import urllib.request
from pathlib import Path

REFERENCES = {
    'frostnova': 'BV1D7411j7sW',
    'patriot': 'BV1Pt4y1m7XK',
    'w': 'BV164411e71d',
    'crownslayer': 'BV114411E7LR',
}
HEADERS = {'User-Agent': 'Mozilla/5.0', 'Referer': 'https://www.bilibili.com/'}


def request(url):
    return urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS), timeout=45)


def api(path, **params):
    with request('https://api.bilibili.com/' + path + '?' + urllib.parse.urlencode(params)) as response:
        result = json.load(response)
    if result.get('code') != 0:
        raise RuntimeError(str(result.get('message')))
    return result['data']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--boss', choices=REFERENCES, required=True)
    parser.add_argument('--ffmpeg', required=True)
    args = parser.parse_args()
    root = Path('output/boss-vfx-reference') / args.boss
    root.mkdir(parents=True, exist_ok=True)
    bvid = REFERENCES[args.boss]
    info = api('x/web-interface/view', bvid=bvid)
    playback = api('x/player/playurl', bvid=bvid, cid=info['cid'], qn=32, fnval=0)
    segments = playback.get('durl', [])
    if len(segments) != 1:
        raise RuntimeError('Expected one publicly accessible reference segment')
    video = root / 'reference.mp4'
    if not video.exists():
        temporary = root / 'reference.part'
        with request(segments[0]['url']) as source, temporary.open('wb') as dest:
            while data := source.read(1024 * 1024):
                dest.write(data)
        temporary.rename(video)
    metadata = {key: info[key] for key in ('bvid', 'cid', 'title', 'duration')}
    metadata.update(url=f'https://www.bilibili.com/video/{bvid}/', quality=playback['quality'],
                    purpose='Local reference analysis only; excluded from game assets')
    (root / 'source.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding='utf8')
    subprocess.run([args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', '-i', str(video),
                    '-vf', 'fps=1/5,scale=480:-1', str(root / 'overview-%03d.jpg')], check=True)
    print(args.boss, 'downloaded', video.stat().st_size, 'bytes', flush=True)


if __name__ == '__main__':
    main()
