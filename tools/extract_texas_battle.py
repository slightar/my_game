"""Extract Texas's front battle rig from the local client, read-only.

The bundle contains both front/back battle rigs and a separate building rig.
Select the front rig by its F_Weapon attachment, never by duplicate asset names.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from extract_client_ui import UnityPy


def script(data):
    value = data.m_Script
    return value.encode('utf-8', 'surrogateescape') if isinstance(value, str) else value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True, help='Developer-only pixi.js/spine.js folder')
    args = parser.parse_args()
    relative = 'chararts/char_102_texas.ab'
    bundle = args.client / relative
    objects = [(o, o.read()) for o in UnityPy.load(str(bundle)).objects
               if o.type.name in ('TextAsset', 'Texture2D')]
    rig = next((o, d) for o, d in objects
               if d.m_Name == 'char_102_texas.skel' and b'F_Weapon' in script(d))
    atlas = next((o, d) for o, d in objects
                 if d.m_Name == 'char_102_texas.atlas' and b'F_Weapon' in script(d))
    texture = next((o, d) for o, d in objects
                   if d.m_Name == 'char_102_texas' and d.m_Width == 512)
    alpha = next((o, d) for o, d in objects
                 if d.m_Name == 'char_102_texas[alpha]' and d.m_Width == 512)
    folder = args.output / 'texas_battle'
    folder.mkdir(parents=True, exist_ok=True)
    for extension, (_, data) in [('skel', rig), ('atlas', atlas)]:
        (folder / ('char_102_texas.' + extension)).write_bytes(script(data))
    image = texture[1].image.convert('RGBA')
    image.putalpha(alpha[1].image.convert('RGB').getchannel('R'))
    image.save(folder / 'char_102_texas.png')
    for name in ['pixi.js', 'spine.js']:
        shutil.copyfile(args.runtime / name, args.output / name)
    (args.output / 'models.json').write_text(json.dumps({'texas_battle': 'char_102_texas'}))
    source = {'bundle': relative, 'bundle_sha256': hashlib.sha256(bundle.read_bytes()).hexdigest(),
              'model': 'char_102_texas', 'view': 'front',
              'objects': {key: str(obj.path_id) for key, (obj, _) in
                          [('skel', rig), ('atlas', atlas), ('texture', texture), ('alpha', alpha)]},
              'skel_sha256': hashlib.sha256(script(rig[1])).hexdigest()}
    (args.output / 'source.json').write_text(json.dumps(source, indent=2))
    print(json.dumps(source, indent=2))


if __name__ == '__main__':
    main()
