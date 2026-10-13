"""Export client particle curves at 60 Hz for the side-view renderer.

This preserves source curve timing, color and texture dependencies. Geometry and
emitter projection remain a documented 2D adaptation, not a Unity simulation.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
from extract_client_ui import UnityPy

ROOTS = {
    'frstar2': ['frstar2_attack_02_start'],
    'crowns': ['crowns_attack_01_start_right'],
    'cqbw': ['cqbw_attack_01_hit_01', 'cqbw_skill_03_hit'],
    'patrt': ['patrt_attack_01_start'],
}


def curve(c, t):
    if c.get('minMaxState', 0) == 0:
        return c.get('scalar', 1)
    keys = c.get('maxCurve', {}).get('m_Curve', [])
    if not keys:
        return c.get('scalar', 1)
    if t <= keys[0]['time']:
        return keys[0]['value'] * c.get('scalar', 1)
    for a, b in zip(keys, keys[1:]):
        if t <= b['time']:
            dt = b['time'] - a['time']
            x = (t-a['time'])/dt
            # Unity's unweighted Hermite tangents, retained from the source.
            value = (2*x**3-3*x*x+1)*a['value'] + (x**3-2*x*x+x)*dt*a['outSlope']
            value += (-2*x**3+3*x*x)*b['value'] + (x**3-x*x)*dt*b['inSlope']
            return value*c.get('scalar', 1)
    return keys[-1]['value']*c.get('scalar', 1)


def color(module, t):
    if not module.get('enabled'):
        return [1,1,1,1]
    g = module['gradient']['maxGradient']
    result = []
    for channel in 'rgba':
        alpha = channel == 'a'
        count = g['m_NumAlphaKeys' if alpha else 'm_NumColorKeys']
        prefix = 'atime' if alpha else 'ctime'
        keys = [(g[prefix+str(i)]/65535, g['key'+str(i)][channel]) for i in range(count)]
        value = keys[-1][1] if keys else 1
        if keys and t <= keys[0][0]: value = keys[0][1]
        for (ta, va), (tb, vb) in zip(keys, keys[1:]):
            if ta <= t <= tb:
                value = va+(vb-va)*(t-ta)/max(.000001,tb-ta)
                break
        result.append(value)
    return result


def main():
    p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True)
    p.add_argument('--output',type=Path,default=Path('assets/enemies/bosses/effects'))
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    shared={str(o.path_id):o.read_typetree() for o in UnityPy.load(str(args.client/'refs/fx/material.ab')).objects if o.type.name=='Material'}
    manifest=json.loads((args.output/'manifest.json').read_text(encoding='utf8'))
    textures=manifest['textures'];wanted={};clips={}
    for bundle,roots in ROOTS.items():
        path=args.client/f'battle/prefabs/effects/{bundle}.ab'
        objects={str(o.path_id):o.read_typetree() for o in UnityPy.load(str(path)).objects
                 if o.type.name in ['GameObject','Transform','ParticleSystem','ParticleSystemRenderer','Material']}
        def ref(ptr):return objects.get(str(ptr.get('m_PathID')),shared.get(str(ptr.get('m_PathID')),{}))
        def walk(tr,layers):
            go=ref(tr.get('m_GameObject',{}))
            components=[ref(v.get('component',v)) for v in go.get('m_Component',[])]
            ps=next((v for v in components if 'InitialModule' in v),None)
            renderer=next((v for v in components if 'm_RenderMode' in v),None)
            if ps and renderer:
                materials=renderer.get('m_Materials',[])
                mat=ref(materials[0]) if materials else {}
                tex=dict(mat.get('m_SavedProperties',{}).get('m_TexEnvs',[])).get('_MainTex',{}).get('m_Texture',{})
                pid=str(tex.get('m_PathID',0))
                emission=ps.get('EmissionModule',{})
                if pid!='0' and emission.get('enabled'):
                    wanted[pid]=None
                    initial=ps['InitialModule'];life=curve(initial['startLifetime'],0)
                    uv=ps.get('UVModule',{});columns=uv.get('tilesX',1) if uv.get('enabled') else 1
                    rows=uv.get('tilesY',1) if uv.get('enabled') else 1
                    size=ps.get('SizeModule',{});base=initial['startColor']['maxColor']
                    frames=[]
                    for i in range(math.ceil(life*60)+1):
                        t=min(1,i/60/max(.001,life));rgba=color(ps.get('ColorModule',{}),t)
                        frames.append([round(max(0,curve(size['curve'],t) if size.get('enabled') else 1),6),
                            *[round(max(0,min(1,rgba[j]*base[k])),6) for j,k in enumerate('rgba')],
                            min(columns*rows-1,int(curve(uv.get('frameOverTime',{}),t)*columns*rows)) if uv.get('enabled') else 0])
                    layers.append({'name':go['m_Name'],'texture_id':pid,'material':mat.get('m_Name'),
                        'life':life,'delay':curve(ps.get('startDelay',{}),0),'size':curve(initial['startSize'],0),
                        'speed':curve(initial['startSpeed'],0),'render_mode':renderer['m_RenderMode'],
                        'columns':columns,'rows':rows,'position':tr.get('m_LocalPosition'),'frames':frames})
            for child in tr.get('m_Children',[]):walk(ref(child),layers)
        for go in objects.values():
            if go.get('m_Name') in roots and 'm_Component' in go:
                layers=[]
                for v in go['m_Component']:
                    tr=ref(v.get('component',v))
                    if 'm_Children' in tr:walk(tr,layers)
                clips[go['m_Name']]=layers
    missing=set(wanted)-textures.keys()
    for path in sorted((args.client/'refs/fx/texture').glob('*.ab')):
        if not missing:break
        for obj in UnityPy.load(str(path)).objects:
            pid=str(obj.path_id)
            if obj.type.name=='Texture2D' and pid in missing:
                data=obj.read();file=data.m_Name+'.png';data.image.save(args.output/file)
                textures[pid]={'name':data.m_Name,'file':file,'bundle':str(path.relative_to(args.client)),
                    'sha256':hashlib.sha256((args.output/file).read_bytes()).hexdigest()}
                missing.remove(pid)
    if missing:raise RuntimeError('Unresolved textures: '+str(missing))
    for layers in clips.values():
        for layer in layers:layer['texture']=textures[layer['texture_id']]['name']
    (args.output/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    result={'fps':60,'source':'Client particle lifetime, start color, ColorModule and SizeModule; deterministic max curves',
            'projection':'2D adaptation; mesh, shader dissolve, random emission and world transforms are not reproduced', 'clips':clips}
    (args.output/'timelines.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    for name,layers in clips.items():print(name,[(v['name'],v['texture'],round(v['life'],2)) for v in layers])


if __name__=='__main__':main()
