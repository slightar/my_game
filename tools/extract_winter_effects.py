"""Export original FrostNova/Patriot VFX textures and their material provenance.
Read-only client access; exported textures retain their original pixels/alpha.
"""
import argparse, hashlib, json
from pathlib import Path
from extract_client_ui import UnityPy

def main():
    p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    texture_objects={};bundles={};prefabs={};wanted=set()
    for key in ['frstar2','patrt','crowns','cqbw']:
        path=args.client/f'battle/prefabs/effects/{key}.ab';env=UnityPy.load(str(path))
        entries=[]
        for obj in env.objects:
            if obj.type.name=='Texture2D':texture_objects[obj.path_id]=obj;bundles[obj.path_id]=path
            elif obj.type.name=='Material':
                tree=obj.read_typetree();tex=dict(tree['m_SavedProperties']['m_TexEnvs'])
                pointer=tex.get('_MainTex',{}).get('m_Texture',{})
                if pointer.get('m_PathID'):
                    wanted.add(pointer['m_PathID'])
                    entries.append({'material':tree['m_Name'],'texture_object':str(pointer['m_PathID']),
                        'properties':tree['m_SavedProperties']})
        prefabs[key]=entries
    # frstar2_attack_02_start keeps three of its particle materials in the shared FX bundle.
    # Read them too, or the ice-ring export contains only the dark crescent (huan).
    ring_layers=[]
    for obj in UnityPy.load(str(args.client/'refs/fx/material.ab')).objects:
        if obj.type.name!='Material':continue
        tree=obj.read_typetree()
        if tree['m_Name'] not in {'trail_08_ab','star_10_ab_ZT','star_02_add_ZT'}:continue
        pointer=dict(tree['m_SavedProperties']['m_TexEnvs']).get('_MainTex',{}).get('m_Texture',{})
        if pointer.get('m_PathID'):
            wanted.add(pointer['m_PathID'])
            ring_layers.append({'material':tree['m_Name'],'texture_object':str(pointer['m_PathID']),
                'properties':tree['m_SavedProperties']})
    prefabs['frstar2_attack_02_start_shared']=ring_layers
    missing=wanted-texture_objects.keys()
    for path in sorted((args.client/'refs/fx/texture').glob('*.ab')):
        if not missing:break
        for obj in UnityPy.load(str(path)).objects:
            if obj.type.name=='Texture2D' and obj.path_id in missing:
                texture_objects[obj.path_id]=obj;bundles[obj.path_id]=path;missing.remove(obj.path_id)
    if missing:raise RuntimeError('Missing client VFX texture dependencies: '+str(missing))
    sources={}
    for pid,obj in texture_objects.items():
        data=obj.read();name=data.m_Name
        data.image.save(args.output/(name+'.png'))
        sources[str(pid)]={'name':name,'file':name+'.png','bundle':str(bundles[pid].relative_to(args.client)),
            'sha256':hashlib.sha256((args.output/(name+'.png')).read_bytes()).hexdigest()}
    (args.output/'manifest.json').write_text(json.dumps({'textures':sources,'materials':prefabs},ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print('Exported',len(sources),'texture objects (',len({entry['name'] for entry in sources.values()}),
          'unique images) with',sum(map(len,prefabs.values())),'material mappings')
if __name__=='__main__':main()
