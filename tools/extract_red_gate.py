"""Read the client's tile_start meshes/materials and bake a front-facing 2D sprite.

No client files are written. Geometry, UVs and texture artwork are exported unchanged;
the offline orthographic projection removes cube perspective for the side-view game.
Requires the same UnityPy/Pillow environment as extract_client_ui.py.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
from PIL import Image
from extract_client_ui import UnityPy

MESH_IDS = [-1748945470632165655, -6633492496227041559, -2180423446155641033]
TEXTURE_ID = 1671617118310560865


def render_front(meshes, texture, size=512, additive=False):
    # Unity's prefab rotation turns the source mesh +Y into the front (-Z).
    # UnityPy's OBJ exporter already negates source X. Project X/Z without depth.
    pixels = Image.new('RGBA', (size, size))
    dst = pixels.load()
    src = texture.convert('RGBA').load()
    tw, th = texture.size
    depth = [-math.inf] * (size * size)
    scale = (size - 16) / 100.33
    for obj in meshes:
        vertices, uv, faces = [], [], []
        for line in obj.splitlines():
            parts = line.split()
            if not parts:
                continue
            if parts[0] == 'v':
                vertices.append(tuple(map(float, parts[1:4])))
            elif parts[0] == 'vt':
                uv.append(tuple(map(float, parts[1:3])))
            elif parts[0] == 'f':
                faces.append([tuple(int(i) - 1 for i in p.split('/')[:2]) for p in parts[1:]])
        for face in faces:
            v = [vertices[i] for i, _ in face]
            t = [uv[j] for _, j in face]
            xy = [(size / 2 + p[0] * scale, size / 2 + p[2] * scale) for p in v]
            (ax, ay), (bx, by), (cx, cy) = xy
            den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
            if abs(den) < .00001:
                continue
            left, right = max(0, math.floor(min(p[0] for p in xy))), min(size - 1, math.ceil(max(p[0] for p in xy)))
            top, bottom = max(0, math.floor(min(p[1] for p in xy))), min(size - 1, math.ceil(max(p[1] for p in xy)))
            for y in range(top, bottom + 1):
                for x in range(left, right + 1):
                    w0 = ((by - cy) * (x + .5 - cx) + (cx - bx) * (y + .5 - cy)) / den
                    w1 = ((cy - ay) * (x + .5 - cx) + (ax - cx) * (y + .5 - cy)) / den
                    w2 = 1 - w0 - w1
                    if min(w0, w1, w2) < -.00001:
                        continue
                    z = sum(w * p[1] for w, p in zip((w0, w1, w2), v))
                    index = y * size + x
                    if z < depth[index]:
                        continue
                    depth[index] = z
                    u = sum(w * p[0] for w, p in zip((w0, w1, w2), t))
                    vv = sum(w * p[1] for w, p in zip((w0, w1, w2), t))
                    # Original atlas, original UVs: no replacement warning sign/frame.
                    color = src[min(tw - 1, max(0, int(u * tw))), min(th - 1, max(0, int((1 - vv) * th)))]
                    # Preserve RGB for the original additive material: alpha-zero
                    # red texels still contribute in that pass. Empty space stays clear.
                    dst[x, y] = (*color[:3], 255) if additive else color
    return pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True, help='StreamingAssets/AB/Windows')
    parser.add_argument('--output', type=Path, default=Path('assets/environment/red_gate'))
    args = parser.parse_args()
    bundle = args.client / 'arts/effects/[pack]map.ab'
    env = UnityPy.load(str(bundle))
    objects = {o.path_id: o for o in env.objects}
    args.output.mkdir(parents=True, exist_ok=True)
    sources = args.output / 'source'
    sources.mkdir(exist_ok=True)
    meshes = []
    for id_ in MESH_IDS:
        model = objects[id_].read()
        obj = model.export()
        meshes.append(obj)
        (sources / (model.m_Name + '.obj')).write_text(obj, encoding='utf-8')
    material_ids = [3342030348989254990, 7118164268692265483]
    for id_ in material_ids:
        material = objects[id_]
        (sources / (material.read().m_Name.replace('[opt]', '') + '.json')).write_text(
            json.dumps(material.read_typetree(), ensure_ascii=False, indent=2), encoding='utf-8')
    texture = objects[TEXTURE_ID].read().image
    texture.save(sources / 'merged_textures.png')
    render_front(meshes, texture).save(args.output / 'entry_front.png')
    render_front(meshes, texture, additive=True).save(args.output / 'entry_add.png')
    metadata = {
        'prefab_bundle': 'battle/prefabs/effects/tile.ab',
        'prefab': 'tile_start', 'prefab_path_id': 3417487498484636784,
        'model_bundle': 'arts/effects/[pack]map.ab',
        'bundle_sha256': hashlib.sha256(bundle.read_bytes()).hexdigest(),
        'mesh_path_ids': MESH_IDS, 'texture_path_id': TEXTURE_ID,
        'projection': 'orthographic front; no perspective or side face',
        'file': 'entry_front.png', 'size': [512, 512],
        'additive_file': 'entry_add.png',
        'runtime_size': 132, 'runtime_alpha': 0.48,
        'note': 'Original mesh/UV/atlas; 2D alpha blending replaces Unity additive passes and particle effects.'
    }
    (args.output / 'manifest.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding='utf-8')
    print(args.output / 'entry_front.png')


if __name__ == '__main__':
    main()
