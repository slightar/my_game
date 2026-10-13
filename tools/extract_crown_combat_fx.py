"""Export existing client artwork and sampled particle curves for Crown combat.

Only reads the installed game. Images keep their original pixels and alpha.
The renderer adapts emitter placement to a side-view arena; it does not claim
to reproduce Unity meshes, shaders or the complete source skills.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

from extract_client_ui import UnityPy
from export_boss_fx_timeline import color, curve

ROOTS = {
    'red': ['red_attack_01_start_r', 'red_skill_02_start'],
    'texas2': ['texas2_skill_02_hit', 'texas2_birth_01_start', 'texas2_skill_03_hit_02'],
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('assets/enemies/bosses/effects/crown_client'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    shared = {str(o.path_id): o.read_typetree()
              for o in UnityPy.load(str(args.client / 'refs/fx/material.ab')).objects
              if o.type.name == 'Material'}
    textures, clips, wanted, skipped, sources = {}, {}, set(), [], {}
    for bundle, roots in ROOTS.items():
        path = args.client / f'battle/prefabs/effects/{bundle}.ab'
        sources[str(path.relative_to(args.client))] = hashlib.sha256(path.read_bytes()).hexdigest()
        env = UnityPy.load(str(path))
        objects = {str(o.path_id): o.read_typetree() for o in env.objects
                   if o.type.name in ['GameObject', 'Transform', 'ParticleSystem', 'ParticleSystemRenderer', 'Material']}
        for o in env.objects:
            if o.type.name == 'Texture2D':
                textures[str(o.path_id)] = (o, path)

        def ref(pointer):
            pid = str(pointer.get('m_PathID'))
            return objects.get(pid, shared.get(pid, {}))

        def walk(transform, layers, root):
            go = ref(transform.get('m_GameObject', {}))
            components = [ref(c.get('component', c)) for c in go.get('m_Component', [])]
            ps = next((c for c in components if 'InitialModule' in c), None)
            renderer = next((c for c in components if 'm_RenderMode' in c), {})
            if ps and ps.get('EmissionModule', {}).get('enabled'):
                materials = renderer.get('m_Materials', [])
                material = ref(materials[0]) if materials else {}
                tex = dict(material.get('m_SavedProperties', {}).get('m_TexEnvs', [])).get('_MainTex', {}).get('m_Texture', {})
                pid = str(tex.get('m_PathID', 0))
                if pid != '0':
                    wanted.add(pid)
                    initial, uv = ps['InitialModule'], ps.get('UVModule', {})
                    life = curve(initial['startLifetime'], 0)
                    if life <= 0:
                        raise ValueError(f'Invalid particle lifetime: {root}/{go["m_Name"]}')
                    columns, rows = (uv.get('tilesX', 1), uv.get('tilesY', 1)) if uv.get('enabled') else (1, 1)
                    base, size = initial['startColor']['maxColor'], ps.get('SizeModule', {})
                    frames = []
                    for i in range(math.ceil(life * 60) + 1):
                        t = min(1, i / 60 / life)
                        rgba = color(ps.get('ColorModule', {}), t)
                        frames.append([
                            round(max(0, curve(size['curve'], t) if size.get('enabled') else 1), 6),
                            *[round(max(0, min(1, rgba[j] * base[k])), 6) for j, k in enumerate('rgba')],
                            min(columns * rows - 1, int(curve(uv.get('frameOverTime', {}), t) * columns * rows)) if uv.get('enabled') else 0,
                        ])
                    name = go['m_Name']
                    if any(layer['name'] == name for layer in layers):
                        name += '_' + str(sum(layer['name'].startswith(name) for layer in layers))
                    layers.append({'name': name, 'texture_id': pid, 'material': material['m_Name'],
                                   'life': life, 'delay': curve(ps.get('startDelay', {}), 0),
                                   'columns': columns, 'rows': rows, 'position': transform.get('m_LocalPosition'),
                                   'rotation': transform.get('m_LocalRotation'), 'frames': frames})
                else:
                    skipped.append({'clip': root, 'layer': go.get('m_Name'),
                                    'reason': 'No resolved billboard texture; mesh/shader layer is not exported'})
            for child in transform.get('m_Children', []):
                walk(ref(child), layers, root)

        for go in objects.values():
            if go.get('m_Name') not in roots or 'm_Component' not in go:
                continue
            layers = []
            for component in go['m_Component']:
                transform = ref(component.get('component', component))
                if 'm_Children' in transform:
                    walk(transform, layers, go['m_Name'])
            clips[go['m_Name']] = layers

    missing = wanted - textures.keys()
    for path in sorted((args.client / 'refs/fx/texture').glob('*.ab')):
        if not missing:
            break
        for obj in UnityPy.load(str(path)).objects:
            pid = str(obj.path_id)
            if obj.type.name == 'Texture2D' and pid in missing:
                textures[pid] = (obj, path)
                missing.remove(pid)
    if missing:
        raise RuntimeError('Missing source textures: ' + str(missing))
    manifest = {'source_bundles': sources, 'textures': {}, 'skipped_layers': skipped,
                'projection': 'Existing source pixels/alpha and 60 Hz curves; 2D placement adaptation, no Unity mesh/shader reconstruction'}
    for pid in sorted(wanted):
        obj, path = textures[pid]
        data = obj.read()
        filename = data.m_Name + '_' + pid + '.png'
        data.image.save(args.output / filename)
        manifest['textures'][pid] = {'name': data.m_Name + '_' + pid, 'original_name': data.m_Name,
                                    'file': filename, 'bundle': str(path.relative_to(args.client)),
                                    'sha256': hashlib.sha256((args.output / filename).read_bytes()).hexdigest()}
    for layers in clips.values():
        for layer in layers:
            layer['texture'] = manifest['textures'][layer['texture_id']]['name']
    (args.output / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    (args.output / 'timelines.json').write_text(json.dumps({'fps': 60, 'clips': clips}, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    print('Exported', len(wanted), 'original textures,', sum(map(len, clips.values())), 'particle layers')
    for name, layers in clips.items():
        print(name, [(v['name'], manifest['textures'][v['texture_id']]['original_name']) for v in layers])


if __name__ == '__main__':
    main()
