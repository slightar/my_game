"""Read-only catalogue/export of original client AudioClips (UnityPy required)."""
import argparse
import hashlib
import json
from pathlib import Path
from extract_client_ui import UnityPy

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--select', type=Path)
    parser.add_argument('--catalogue', type=Path, help='Known clip locations; avoids opening unrelated bundles')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    selection = json.loads(args.select.read_text(encoding='utf-8')) if args.select else None
    catalogue = []
    names = set(selection.values()) if selection else None
    bundles = sorted((args.client/'audio/sound_beta_2').rglob('*.ab'))
    if args.catalogue and selection:
        entries = json.loads(args.catalogue.read_text(encoding='utf-8'))
        bundles = sorted({args.client/c['bundle'] for c in entries if c['name'] in names})
    for bundle in bundles:
        if not selection and not bundle.name.startswith(('general_', 'btl_snd_', 'p_atk_', 'p_skill_', 'p_imp_')):
            continue
        env = UnityPy.load(str(bundle))
        for obj in env.objects:
            if obj.type.name != 'AudioClip':
                continue
            clip = obj.read()
            if names is not None and clip.m_Name not in names:
                continue
            entry = {'name': clip.m_Name, 'bundle': str(bundle.relative_to(args.client)),
                     'object': obj.path_id, 'duration': clip.m_Length,
                     'channels': clip.m_Channels, 'frequency': clip.m_Frequency}
            if selection:
                entry['bundleSha256'] = hashlib.sha256(bundle.read_bytes()).hexdigest()
                samples = clip.samples
                if len(samples) != 1:
                    raise RuntimeError(f'Unexpected sample count: {clip.m_Name}: {len(samples)}')
                source_name, data = next(iter(samples.items()))
                suffix = Path(source_name).suffix
                for key, name in selection.items():
                    if name != clip.m_Name:
                        continue
                    path = args.output/(key+suffix)
                    path.write_bytes(data)
                    entry.setdefault('exports', []).append(path.name)
                    entry['sampleSha256'] = hashlib.sha256(data).hexdigest()
                print(clip.m_Name, entry.get('exports'))
            catalogue.append(entry)
    if selection and names != {c['name'] for c in catalogue}:
        raise RuntimeError(f'Missing clips: {names-{c["name"] for c in catalogue}}')
    (args.output/('sources.json' if selection else 'catalogue.json')).write_text(
        json.dumps(catalogue, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'{len(catalogue)} clips')

if __name__ == '__main__':
    main()
