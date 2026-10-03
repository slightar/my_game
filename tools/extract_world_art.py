"""Read-only export of original client scene sprites and Chernobog atlas."""
import argparse
import hashlib
import json
from pathlib import Path
from extract_client_ui import UnityPy

SELECTION = {
    'in': ['bg_infirmary'],
    'ch': ['bg_cher_0', 'bg_cher_3', 'bg_cher_5', 'bg_cher_9', 'bg_cher_11',
           'bg_cherunder', 'bg_cherunder_2', 'bg_chercen_2', 'bg_cher_4'],
    'un': ['bg_undergroud_f', 'bg_undergroud_n', 'bg_undergroundF'],
    'to': ['bg_towerinside', 'bg_top', 'bg_topburning'],
}
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = {}
    sources = [(f'avg/bg/avg_bkg_h1_bg_{key}_0.ab', names, 'Sprite') for key, names in SELECTION.items()]
    sources.append(('arts/maps/map_chernobog_a/res.ab', ['TX_Qcity_Common_A'], 'Texture2D'))
    for bundle, names, kind in sources:
        path = args.client / bundle
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        for obj in UnityPy.load(str(path)).objects:
            if obj.type.name != kind:
                continue
            data = obj.read()
            if data.m_Name not in names:
                continue
            target = args.output / (data.m_Name + '.png')
            data.image.save(target)
            manifest[data.m_Name] = {'bundle': bundle, 'object': obj.path_id, 'sha256': digest,
                                     'size': data.image.size, 'type': kind}
    if len(manifest) != sum(len(names) for _, names, _ in sources):
        raise RuntimeError('Missing expected source artwork')
    (args.output / 'sources.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'Exported {len(manifest)} original scene assets')
if __name__ == '__main__':
    main()
