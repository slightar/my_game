"""Export W's original muzzle and projectile layers using the shared client reader.

Usage: python tools/extract_w_combat_fx.py --client <Windows AB directory>
Pixels and sampled curves are preserved; emitter placement is adapted in C++.
"""
import sys
import extract_crown_combat_fx as exporter

exporter.ROOTS = {'cqbw': ['cqbw_attack_01_start', 'cqbw_attack_01_start_02',
                          'cqbw_attack_01_trail_01']}

if __name__ == '__main__':
    if '--output' not in sys.argv:
        sys.argv.extend(['--output', 'assets/enemies/bosses/effects/w_client'])
    exporter.main()
