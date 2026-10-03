"""Verify original audio exports against their manifest and check decoded PCM."""
import array
import hashlib
import json
import math
import wave
from pathlib import Path

root = Path(__file__).resolve().parents[1] / 'assets/audio/client'
sources = json.loads((root/'sources.json').read_text(encoding='utf-8'))
selection = json.loads((root/'selection.json').read_text(encoding='utf-8'))
seen = set()
report = []
for source in sources:
    for exported in source['exports']:
        path = root/exported
        assert selection[path.stem] == source['name'], path
        assert hashlib.sha256(path.read_bytes()).hexdigest() == source['sampleSha256'], path
        with wave.open(str(path), 'rb') as audio:
            assert audio.getsampwidth() == 2 and audio.getcomptype() == 'NONE', path
            duration = audio.getnframes()/audio.getframerate()
            assert abs(duration-source['duration']) < .03, path
            samples = array.array('h', audio.readframes(audio.getnframes()))
            peak = max(abs(n) for n in samples)/32768
            rms = math.sqrt(sum(n*n for n in samples)/len(samples))/32768
            assert rms > 1e-5 and peak > .001, f'Silent clip: {path}'
            report.append({'cue': path.stem, 'original': source['name'],
                           'seconds': round(duration, 3), 'peak': round(peak, 4),
                           'rms': round(rms, 4)})
        seen.add(path.stem)
assert seen == set(selection), 'Missing or unexpected export'
output = root.parents[2]/'output/client-audio-qa'
output.mkdir(parents=True, exist_ok=True)
(output/'pcm-validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(f'{len(report)} unmodified client exports: SHA-256, duration and non-silent PCM verified')
