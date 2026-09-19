"""Read local client models for offline baking; never writes to the client installation.
Developer tool only: uses the project's existing UnityPy extraction environment.
"""
import argparse
from pathlib import Path
import json
from extract_client_ui import UnityPy  # Registers the client's bundle decompressor.

MODELS = {
    'slug': ('enemy_1007_slime', 'enm_art_base_0.ab'),
    'soldier': ('enemy_1002_nsabr', 'enm_art_base_0.ab'),
    'crossbow': ('enemy_1003_ncbow', 'enm_art_base_0.ab'),
    'exploder': ('enemy_1021_bslime', 'enm_art_3.ab'),
    'shield': ('enemy_1006_shield', 'enm_art_base_0.ab'),
    'drone': ('enemy_1005_yokai', 'enm_art_base_0.ab'),
}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True, help='StreamingAssets/AB/Windows')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for bundle in sorted({entry[1] for entry in MODELS.values()}):
        objects = {}
        for obj in UnityPy.load(str(args.client / 'refs/arts' / bundle)).objects:
            if obj.type.name in ('Texture2D', 'TextAsset'):
                data = obj.read()
                objects[data.m_Name] = data
        for key, (model, source) in MODELS.items():
            if source != bundle:
                continue
            folder = args.output / key
            folder.mkdir(exist_ok=True)
            for extension in ('atlas', 'skel'):
                data = objects[f'{model}.{extension}'].m_Script
                if isinstance(data, str):
                    data = data.encode('utf-8', errors='surrogateescape')
                (folder / f'{model}.{extension}').write_bytes(data)
            image = objects[model].image.convert('RGBA')
            alpha = objects.get(model + '[alpha]')
            if alpha:
                image.putalpha(alpha.image.convert('RGB').getchannel('R'))
            image.save(folder / f'{model}.png')
            print(key, image.size, flush=True)
    (args.output / 'models.json').write_text(json.dumps({k: v[0] for k, v in MODELS.items()}))

if __name__ == '__main__':
    main()
