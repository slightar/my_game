"""Export chapter-matched enemy rigs; all client reads are read-only.

Use the exact enemy ID, including _2 variants, instead of folding skin suffixes.
--source contains the original stage JSON and enemy handbook JSON downloaded
from Kengxxiao/ArknightsGameData (see chapter_rosters.json for source links).
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from extract_client_ui import UnityPy

MODELS = {
    'slug_alpha': 'enemy_1007_slime_2', 'mobile_shield': 'enemy_1029_shdsbr',
    'hound_pro': 'enemy_1000_gopro_2', 'dual_swordsman': 'enemy_1014_rogue',
    'molotov': 'enemy_1028_mocock_2', 'scavenger': 'enemy_1030_wteeth',
    'snow_soldier': 'enemy_1064_snsbr', 'snow_sniper': 'enemy_1066_snbow',
    'ice_slug': 'enemy_1067_snslime', 'icebreaker': 'enemy_1069_icebrk',
    'snow_caster_leader': 'enemy_1068_snmage_2',
    'guerrilla_hound': 'enemy_1077_sotihd', 'guerrilla_fighter': 'enemy_1078_sotisc',
    'guerrilla_sniper': 'enemy_1079_sotisp', 'guerrilla_mortar': 'enemy_1082_soticn',
    'guerrilla_raider': 'enemy_1083_sotiab', 'guerrilla_sarkaz': 'enemy_1084_sotidm',
    'ursus_beast': 'enemy_1108_uterer', 'ursus_caster': 'enemy_1110_uamord',
    'ursus_assault': 'enemy_1111_ucommd',
}
STAGES = {'bridge':'1-8','wtower':'1-12','ice':'6-16','industry':'7-18','core':'JT8-3'}


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--runtime',type=Path,required=True)
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    handbook=json.loads((args.source/'enemy_handbook_table.json').read_text(encoding='utf-8'))['enemyData']
    pending=dict(MODELS);metadata={}
    for bundle in sorted((args.client/'refs/arts').glob('enm_art_*.ab')):
        if not pending:break
        objects={}
        for obj in UnityPy.load(str(bundle)).objects:
            if obj.type.name in ('TextAsset','Texture2D'):
                d=obj.read();objects[d.m_Name]=(obj,d)
        for key,model in list(pending.items()):
            if not all(model+ext in objects for ext in ['.skel','.atlas','']):continue
            folder=args.output/key;folder.mkdir(exist_ok=True)
            ids={}
            for ext in ['.skel','.atlas']:
                obj,d=objects[model+ext];data=d.m_Script
                if isinstance(data,str):data=data.encode('utf-8','surrogateescape')
                (folder/(model+ext)).write_bytes(data);ids[ext]=str(obj.path_id)
            obj,d=objects[model];image=d.image.convert('RGBA');ids['texture']=str(obj.path_id)
            if model+'[alpha]' in objects:
                obj,a=objects[model+'[alpha]'];ids['alpha']=str(obj.path_id)
                image.putalpha(a.image.convert('RGB').getchannel('R'))
            image.save(folder/(model+'.png'))
            metadata[key]={'model':model,'name':handbook[model]['name'],
                'bundle':'refs/arts/'+bundle.name,'objects':ids,
                'skel_sha256':hashlib.sha256((folder/(model+'.skel')).read_bytes()).hexdigest()}
            print(key,metadata[key]['name'],bundle.name,flush=True);del pending[key]
    if pending:raise RuntimeError('Missing exact client models: '+str(pending))
    for name in ['pixi.js','spine.js']:shutil.copyfile(args.runtime/name,args.output/name)
    (args.output/'models.json').write_text(json.dumps(MODELS),encoding='utf-8')
    (args.output/'sources.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
    stages=json.loads((args.source/'stage_table.json').read_text(encoding='utf-8'))['stages']
    chapters={}
    for region,code in STAGES.items():
        stage=next(v for k,v in stages.items() if k.startswith('main_') and '#' not in k and v.get('code')==code)
        level=json.loads((args.source/(code+'.json')).read_text(encoding='utf-8'))
        chapters[region]={'code':code,'name':stage['name'],'levelId':stage['levelId'],
            'wiki':'https://prts.wiki/w/'+code,
            'source':'https://github.com/Kengxxiao/ArknightsGameData/blob/master/zh_CN/gamedata/levels/'+stage['levelId'].lower()+'.json',
            'originalEnemies':[{'id':x['id'],'name':handbook[x['id']]['name']} for x in level['enemyDbRefs']],
            'source_sha256':hashlib.sha256((args.source/(code+'.json')).read_bytes()).hexdigest()}
    (args.output/'chapter_rosters.json').write_text(json.dumps(chapters,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':main()
