"""Read-only export of original client loops used by non-boss screens."""
import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path
from extract_client_ui import UnityPy


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--client', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--ffmpeg', type=Path, required=True)
    p.add_argument('--tracks', nargs='+', choices=['home', 'planning', 'quiet', 'battle'])
    a = p.parse_args()
    selections = {
        'home': ('beta1_180603/m_sys_void.ab', 'm_sys_void_loop'),
        'planning': ('beta1_180603/m_dia_darkness02.ab', 'm_dia_darkness02_loop'),
        'quiet': ('beta1_180603/m_dia_darkness01.ab', 'm_dia_darkness01_loop'),
        'battle': ('beta2_180603/m_bat_normalbattle.ab', 'm_bat_normalbattle_loop'),
    }
    a.output.mkdir(parents=True, exist_ok=True)
    manifest = a.output / 'sources.json'
    sources = json.loads(manifest.read_text(encoding='utf-8')) if manifest.exists() else {}
    for name, (relative, clip_name) in selections.items():
        if a.tracks and name not in a.tracks:
            continue
        bundle = a.client / 'audio/sound_beta_2/music' / relative
        clips = [o.read() for o in UnityPy.load(str(bundle)).objects if o.type.name == 'AudioClip']
        clip = next(c for c in clips if c.m_Name == clip_name)
        _, data = next(iter(clip.samples.items()))
        dest = a.output / (name + '.ogg')
        with tempfile.TemporaryDirectory(prefix='page-music-') as tmp:
            wav = Path(tmp) / 'original.wav'
            wav.write_bytes(data)
            subprocess.run([str(a.ffmpeg), '-loglevel', 'error', '-y', '-i', str(wav),
                            '-c:a', 'libvorbis', '-q:a', '5', str(dest)], check=True)
        sources[name] = {'clip': clip_name, 'bundle': str(bundle.relative_to(a.client)),
                         'duration': clip.m_Length, 'sample_sha256': hashlib.sha256(data).hexdigest(),
                         'file': dest.name, 'encoding': 'Vorbis q5'}
        print(name, round(clip.m_Length, 2), dest.stat().st_size, flush=True)
    (a.output / 'sources.json').write_text(json.dumps(sources, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
