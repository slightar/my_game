"""Read the local client's stage sprites without altering the installation.
Uses the existing developer UnityPy environment; the game only ships PNG/JSON.
"""
import argparse
import json
from pathlib import Path
from extract_client_ui import UnityPy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True, help='StreamingAssets/AB/Windows')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads((args.output / 'sources.json').read_text(encoding='utf-8'))
    for bundle in sorted({entry['bundle'] for entry in manifest.values()}):
        names = {entry['sprite']: name for name, entry in manifest.items() if entry['bundle'] == bundle}
        for obj in UnityPy.load(str(args.client / bundle)).objects:
            if obj.type.name != 'Sprite':
                continue
            data = obj.read()
            if data.m_Name not in names:
                continue
            target = args.output / (names[data.m_Name] + '.png')
            # Retain existing assets. Use a fresh output folder + copied manifest to rebuild.
            if not target.exists():
                data.image.save(target)
                print(target.name)


if __name__ == '__main__':
    main()
