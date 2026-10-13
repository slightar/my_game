"""Read-only export of the original Winter Scar and Patriot Spine rigs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from extract_client_ui import UnityPy

MODELS = {'frostnova': 'enemy_1510_frstar2', 'patriot': 'enemy_1506_patrt'}

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--client', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--runtime', type=Path, required=True, help='Directory containing pixi.js and spine.js')
    args = p.parse_args()
    bundle = 'refs/arts/enm_art_16.ab'
    objects = {}
    for obj in UnityPy.load(str(args.client / bundle)).objects:
        if obj.type.name in ('TextAsset', 'Texture2D'):
            d = obj.read()
            objects[d.m_Name] = (obj, d)
    metadata = {}
    for key, model in MODELS.items():
        folder = args.output / key
        folder.mkdir(parents=True, exist_ok=True)
        ids = {}
        for ext in ('.skel', '.atlas'):
            obj, data = objects[model + ext]
            raw = data.m_Script
            if isinstance(raw, str): raw = raw.encode('utf-8', 'surrogateescape')
            (folder / (model + ext)).write_bytes(raw)
            ids[ext] = str(obj.path_id)
        obj, data = objects[model]
        image = data.image.convert('RGBA')
        ids['texture'] = str(obj.path_id)
        if model + '[alpha]' in objects:
            obj, alpha = objects[model + '[alpha]']
            image.putalpha(alpha.image.convert('RGB').getchannel('R'))
            ids['alpha'] = str(obj.path_id)
        image.save(folder / (model + '.png'))
        metadata[key] = {'model': model, 'bundle': bundle, 'objects': ids,
                         'skel_sha256': hashlib.sha256((folder / (model + '.skel')).read_bytes()).hexdigest()}
    for name in ('pixi.js', 'spine.js'): shutil.copyfile(args.runtime / name, args.output / name)
    (args.output / 'models.json').write_text(json.dumps(MODELS), encoding='utf-8')
    (args.output / 'sources.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    print('Exported original Winter Scar and Patriot rigs; client files unchanged.')

if __name__ == '__main__': main()
