"""Validate and merge baked client rigs into the existing Boss manifest."""
import json
from pathlib import Path
import shutil
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = root / 'output/winter-bosses/sheets'
dest = root / 'assets/enemies/bosses'
metadata = json.loads((source / 'manifest.json').read_text(encoding='utf-8'))
sources = json.loads((root / 'output/winter-bosses/models/sources.json').read_text(encoding='utf-8'))
merged = json.loads((dest / 'manifest.json').read_text(encoding='utf-8'))
for key, value in metadata.items():
    image = Image.open(source / f'{key}.png')
    cell = value['cell']
    assert image.size == (20 * cell, value['rows'] * cell), (key, image.size)
    for row in range(6 if key == 'patriot' else 3):
        for col in range(20):
            assert image.crop((col*cell, row*cell, (col+1)*cell, (row+1)*cell)).getchannel('A').getbbox(), (key, row, col)
    bbox = image.crop((0, 0, cell, cell)).getchannel('A').getbbox()
    value['visible_height'] = 115 if key == 'frostnova' else 175
    value['sprite_size'] = value['visible_height'] * cell / (bbox[3] - bbox[1])
    value['source'] = sources[key]
    value['reference'] = 'https://prts.wiki/w/' + ('霜星，“冬痕”' if key == 'frostnova' else '爱国者')
    shutil.copy2(source / f'{key}.png', dest / f'{key}.png')
    merged[key] = value
(dest / 'manifest.json').write_text(json.dumps(merged, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
smooth_source = root / 'output/winter-bosses/smooth'
smooth = json.loads((smooth_source / 'manifest.json').read_text(encoding='utf-8'))
assert smooth['model'] == merged['frostnova']['model'] and smooth['fps'] >= 30
assert smooth['source']['skel_sha256'] == merged['frostnova']['source']['skel_sha256']
with Image.open(smooth_source / 'frostnova.png') as atlas:
    assert atlas.size == (smooth['width'], smooth['height'])
shutil.copy2(smooth_source / 'frostnova.png', dest / 'frostnova_smooth.png')
(dest / 'frostnova_smooth.json').write_text(json.dumps(smooth, ensure_ascii=False, separators=(',', ':')) + '\n', encoding='utf-8')
print('Installed validated Winter Scar and Patriot sheets; existing Bosses preserved.')
