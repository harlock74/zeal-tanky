#!/usr/bin/env python3
"""Compile actual game logic with host sanitizers; SDK functions are stubbed."""
import argparse
import subprocess
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--update-snapshot',action='store_true',help='Explicitly refresh docs/visual-state.bin')
args=parser.parse_args()
for name in ('tiles.zts','tiles.ztp'):
    if not (ROOT/'build/assets'/name).is_file():
        parser.exit(1, 'Missing build/assets/'+name+'; run `zde cmake` first.\n')
subprocess.run(['python3',str(ROOT/'tests/editor_checks.py')],check=True)
with tempfile.TemporaryDirectory(prefix='ztank-checks-') as directory:
    binary=Path(directory)/'game-checks'
    subprocess.run(['python3',str(ROOT/'tools/gen_arenas.py'),str(ROOT/'assets/arenas.tmx'),str(Path(directory)/'arenas.generated.h')],check=True)
    subprocess.run(['python3',str(ROOT/'tools/gen_splash.py'),str(ROOT/'assets/splash.tmx'),str(Path(directory)/'splash.generated.h')],check=True)
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wconversion','-Wsign-conversion','-Wno-error=conversion','-Wno-error=sign-conversion','-fsanitize=address,undefined',
        '-I'+str(ROOT/'src'),'-I'+directory,str(ROOT/'tests/game_checks.c'),str(ROOT/'tests/z80_regressions.c'),
        *(str(ROOT/'src'/f) for f in ['gameplay.c','ai.c','arena.c','physics.c','timing.c']),'-lm','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    for name in ['input','audio']:
        binary=Path(directory)/(name+'-checks')
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wconversion','-Wsign-conversion','-Wno-error=conversion','-Wno-error=sign-conversion','-fsanitize=address,undefined',
            '-I'+str(ROOT/'tests/include'),'-I'+str(ROOT/'src'),str(ROOT/'tests'/f'{name}_checks.c'),str(ROOT/'src/timing.c'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    binary=Path(directory)/'visual-checks'
    subprocess.run(['python3',str(ROOT/'tools/gen_tiles.py'),str(ROOT/'build/assets/tiles.zts'),str(Path(directory)/'tiles.generated.h'),str(Path(directory)/'tiles.rle')],check=True)
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wconversion','-Wsign-conversion','-Wno-error=conversion','-Wno-error=sign-conversion','-fsanitize=address,undefined','-DHOST_CHECK',
        '-I'+str(ROOT/'tests/include'),'-I'+str(ROOT/'src'),'-I'+directory,str(ROOT/'tests/visual_checks.c'),
        *(str(ROOT/'src'/f) for f in ['assets.c','audio.c','render.c','splash.c','gameplay.c','ai.c','arena.c','physics.c','timing.c']),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(ROOT/'build/assets/tiles.zts'),str(ROOT/'build/assets/tiles.ztp'),str(Path(directory)/'tiles.rle'),str(ROOT/'docs/visual-state.bin' if args.update_snapshot else Path(directory)/'visual-state.bin')],check=True)
