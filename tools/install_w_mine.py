"""Install a baked original mine pose without modifying existing source textures."""
import hashlib
import json
import shutil
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = root / 'output/w-mine-bake/render'
dest = root / 'assets/enemies/bosses/effects'
metadata = json.loads((source / 'manifest.json').read_text(encoding='utf-8'))['w_mine']
image = Image.open(source / 'w_mine.png').convert('RGBA')
assert image.size == (128, 128) and image.getchannel('A').getbbox()
assert metadata['pose'] == 'Idle' and metadata['anchor'] == [64, 120]
shutil.copyfile(source / 'w_mine.png', dest / 'w_mine.png')
manifest = json.loads((dest / 'manifest.json').read_text(encoding='utf-8'))
manifest['textures']['w_mine_idle'] = {
    'name': 'w_mine', 'file': 'w_mine.png', 'bundle': metadata['bundle'],
    'sha256': hashlib.sha256((dest / 'w_mine.png').read_bytes()).hexdigest(),
    'baked_model': metadata,
}
(dest / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('Installed complete original W mine pose with source provenance')
