"""Export UI sprites from a locally installed client (UnityPy required).

Flag-4 format reference: https://spottori.com/blog/for-kaltsit-i-reversed-arknights-client/
Only reads bundles; never modifies the installed client.
"""
import sys
from pathlib import Path
import UnityPy
from UnityPy.helpers import CompressionHelper


def decode(data, uncompressed_size):
    result = bytearray()
    pos = 0
    def length(base):
        nonlocal pos
        if base == 15:
            while True:
                value = data[pos]
                pos += 1
                base += value
                if value != 255:
                    break
        return base
    while pos < len(data):
        token = data[pos]
        pos += 1
        count = length(token & 15)
        result.extend(data[pos:pos + count])
        pos += count
        if pos == len(data):
            break
        offset = int.from_bytes(data[pos:pos + 2], 'big')
        pos += 2
        count = length(token >> 4) + 4
        if offset <= 0 or offset > len(result):
            raise ValueError('Invalid match offset')
        for _ in range(count):
            result.append(result[-offset])
        if len(result) > uncompressed_size:
            raise ValueError('Output exceeds declared size')
    if len(result) != uncompressed_size:
        raise ValueError('Decoded size mismatch')
    return bytes(result)


CompressionHelper.DECOMPRESSION_MAP[4] = decode
if __name__ == '__main__':
    root = next(Path('C:/Users/34844').glob('*/Arknights bilibili/games/Arknights/Arknights_Data/StreamingAssets/AB/Windows'))
    output = Path('cmake-build-debug/ui-reference')
    for relative in sys.argv[1:]:
        env = UnityPy.load(str(root / relative))
        folder = output / Path(relative).stem
        folder.mkdir(parents=True, exist_ok=True)
        for obj in env.objects:
            if obj.type.name in ('Sprite', 'Texture2D'):
                try:
                    item = obj.read()
                    name = ''.join(c if c.isalnum() or c in '_-.' else '_' for c in item.m_Name)
                    path = folder / f'{name}_{obj.path_id}.png'
                    item.image.save(path)
                    print(path, item.image.size)
                except Exception as exc:
                    print('SKIP', obj.path_id, str(exc)[:120])
