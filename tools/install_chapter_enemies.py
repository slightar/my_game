"""Install baked chapter models, retain their source identities and check stage membership."""
import json
import shutil
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
work = root / 'output/chapter-enemies'
dest = root / 'assets/enemies/mobs'
metadata = json.loads((work / 'sheets/manifest.json').read_text(encoding='utf-8'))
sources = json.loads((work / 'models/sources.json').read_text(encoding='utf-8'))
manifest = json.loads((dest / 'manifest.json').read_text(encoding='utf-8'))
for key, value in metadata.items():
    image = Image.open(work / 'sheets' / f'{key}.png')
    assert image.size == (1920, 768)
    for row in range(3):
        for col in range(10):
            assert image.crop((col*192, row*192, (col+1)*192, (row+1)*192)).getchannel('A').getbbox(), (key, row, col)
    value['source'] = sources[key]
    assert value['model'] == sources[key]['model']
    box = image.crop((0, 0, 192, 192)).getchannel('A').getbbox()
    value['visible_height'] = round(.29976 * (box[3]-box[1]) / value['scale'], 1)
    value['sprite_size'] = round(.29976 * 192 / value['scale'], 1)
    shutil.copy2(work / 'sheets' / f'{key}.png', dest / f'{key}.png')
    manifest[key] = value
(dest / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
shutil.copy2(work / 'models/chapter_rosters.json', dest / 'chapter_rosters.json')
shutil.copy2(work / 'models/sources.json', dest / 'chapter_sources.json')
print(f'Installed {len(metadata)} verified original client models; {len(manifest)} total sheets.')
