"""Extract exact client loop/cue samples; transcode music to streamable Vorbis."""
import argparse, hashlib, json, subprocess, tempfile
from pathlib import Path
from extract_client_ui import UnityPy

def main():
    p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--ffmpeg',type=Path,required=True);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True)
    selections={
        'frost_phase1':('music/beta2_180603/m_bat_frostnova.ab','m_bat_frostnova_loop'),
        'frost_phase2':('music/static/m_bat/m_bat_frostnovaevolution.ab','m_bat_frostnovaevolution_loop'),
        'patriot':('music/act9d2d0/m_bat_patriot.ab','m_bat_patriot_loop'),
        'ice_ring':('enemy/e_aoe/e_aoe_frostnovaice_h1.ab','e_aoe_frostnovaice_h1'),
        'ice_seal':('enemy/e_aoe/e_aoe_frostnovaice_h2.ab','e_aoe_frostnovaice_h2')}
    sources={}
    for name,(rel,clipname) in selections.items():
        bundle=a.client/'audio/sound_beta_2'/rel
        clips=[o.read() for o in UnityPy.load(str(bundle)).objects if o.type.name=='AudioClip']
        clip=next(c for c in clips if c.m_Name==clipname)
        original,data=next(iter(clip.samples.items()))
        dest=a.output/(name+('.ogg' if name in ['frost_phase1','frost_phase2','patriot'] else '.wav'))
        if dest.suffix=='.ogg':
            with tempfile.TemporaryDirectory(prefix='boss-audio-') as tmp:
                wav=Path(tmp)/'original.wav';wav.write_bytes(data)
                subprocess.run([str(a.ffmpeg),'-loglevel','error','-y','-i',str(wav),'-c:a','libvorbis','-q:a','5',str(dest)],check=True)
        else:dest.write_bytes(data)
        sources[name]={'clip':clipname,'bundle':str(bundle.relative_to(a.client)),'duration':clip.m_Length,
            'sample_sha256':hashlib.sha256(data).hexdigest(),'file':dest.name,'encoding':'Vorbis q5' if dest.suffix=='.ogg' else 'original PCM'}
        print(dest.name,dest.stat().st_size,flush=True)
    (a.output/'sources.json').write_text(json.dumps(sources,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
if __name__=='__main__':main()
