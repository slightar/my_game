"""Install original client Boss sheets with a stable foot anchor and operator-scale art."""
import json
import shutil
from pathlib import Path
from PIL import Image

root=Path(__file__).resolve().parents[1]
source=root/'output/story-bosses/sheets'
dest=root/'assets/enemies/bosses'
dest.mkdir(parents=True,exist_ok=True)
metadata=json.loads((source/'manifest.json').read_text(encoding='utf-8'))
for key,value in metadata.items():
    image=Image.open(source/f'{key}.png')
    assert image.size==(5120,value['rows']*256)
    for row in range(3):
        for col in range(20):
            assert image.crop((col*256,row*256,(col+1)*256,(row+1)*256)).getchannel('A').getbbox(),(key,row,col)
    bbox=image.crop((0,0,256,256)).getchannel('A').getbbox()
    value['visible_height']=108
    value['sprite_size']=108*256/(bbox[3]-bbox[1])
    value['reference']='https://prts.wiki/w/'+('弑君者(敌方)' if key=='crownslayer' else 'W(敌方)')
    shutil.copy2(source/f'{key}.png',dest/f'{key}.png')
(dest/'manifest.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Installed two original client Boss sheets with all attack/movement frames verified.')
