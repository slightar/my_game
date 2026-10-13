"""Read W's original complete mine rig; the cqbw_04 FX map contains debris.

Use bake_enemy_sheets.py --page tools/bake_w_mine.html to bake the idle pose.
"""
import argparse
import hashlib
import json
import shutil
from pathlib import Path
from extract_client_ui import UnityPy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    args = parser.parse_args()
    bundle = 'skinpack/token_10008_cqbw_box.ab'
    model = 'token_10008_cqbw_box_epoque#7'
    objects = {}
    for obj in UnityPy.load(str(args.client / bundle)).objects:
        if obj.type.name in ('TextAsset', 'Texture2D'):
            data = obj.read()
            objects[data.m_Name] = (obj, data)
    dest = args.output / 'w_mine'
    dest.mkdir(parents=True, exist_ok=True)
    ids = {}
    for ext in ('.skel', '.atlas'):
        obj, data = objects[model + ext]
        raw = data.m_Script
        if isinstance(raw, str):
            raw = raw.encode('utf-8', 'surrogateescape')
        if ext == '.atlas':
            raw = raw.replace((model + '.png').encode(), b'w_mine.png')
        (dest / ('w_mine' + ext)).write_bytes(raw)
        ids[ext] = str(obj.path_id)
    obj, data = objects[model]
    image = data.image.convert('RGBA')
    ids['texture'] = str(obj.path_id)
    obj, alpha = objects[model + '[alpha]']
    image.putalpha(alpha.image.convert('RGB').getchannel('R'))
    image.save(dest / 'w_mine.png')
    ids['alpha'] = str(obj.path_id)
    sources = {'w_mine': {'model': model, 'bundle': bundle, 'objects': ids,
        'bundle_sha256': hashlib.sha256((args.client / bundle).read_bytes()).hexdigest(),
        'skel_sha256': hashlib.sha256((dest / 'w_mine.skel').read_bytes()).hexdigest(),
        'projection': 'Original W Epoque mine rig idle pose rendered as a transparent side-view sprite'}}
    for name in ('pixi.js', 'spine.js'):
        shutil.copyfile(args.runtime / name, args.output / name)
    (args.output / 'models.json').write_text(json.dumps({'w_mine': 'w_mine'}), encoding='utf-8')
    (args.output / 'sources.json').write_text(json.dumps(sources, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
