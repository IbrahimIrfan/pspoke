import os
"""Stage a native application test without touching actual saves or the SD card."""
from pathlib import Path
import argparse, datetime, hashlib, json, os, shutil, subprocess

HERE = Path(__file__).resolve().parent
DEFAULT_ROM = Path(os.environ.get('SOULSILVER_ROM', 'SoulSilver.nds'))
DEFAULT_PPSSPP = Path('@PPSSPP@')

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
parser.add_argument('--ppsspp', type=Path, default=DEFAULT_PPSSPP)
parser.add_argument('--input-script', type=Path, help='Optional test-only controller transitions; copied into disposable stage')
parser.add_argument('--frames', type=int, help='Optional diagnostic game-frame bound (1..100000)')
parser.add_argument('--seconds', type=int, default=8)
parser.add_argument('--fixture', type=Path, help='Existing synthetic fixture under this probe runs directory; copied, never modified')
args = parser.parse_args()
rom, emulator = args.rom.resolve(strict=True), args.ppsspp.resolve(strict=True)
eboot = (HERE / 'EBOOT.PBP').resolve(strict=True)
if not 1 <= args.seconds <= 600:
    parser.error('--seconds must be 1..600 wall-clock seconds')
# New directory and exclusive save creation: existing game saves are never read.
run = HERE / 'runs' / datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
stage = run / 'memstick/PSP/GAME/NativeSoulSilver'
stage.mkdir(parents=True, exist_ok=False)
shutil.copyfile(eboot, stage / 'EBOOT.PBP')
manifest=HERE/'build-manifest.json'
if manifest.exists():
    recorded=json.loads(manifest.read_text())
    match=next((item for item in recorded['files'] if item['path']==str(eboot)),None)
    if match and match['sha256']==digest(eboot):shutil.copyfile(manifest,run/'build-manifest.json')
if args.frames is not None:
    if not 1<=args.frames<=100000: parser.error('frame bound must be 1..100000')
    (stage/'probe-frame-limit.txt').write_text(str(args.frames)+'\n')
if args.input_script:
    source=args.input_script.resolve(strict=True)
    if not source.is_relative_to(HERE.parent.resolve()):
        parser.error('input script must be under this isolated native core workspace')
    shutil.copyfile(source, stage/'input-replay.txt')
(stage / 'SoulSilver.nds').symlink_to(rom)
save = stage / 'SoulSilver.native.sav'
fixture_data = b'\xff' * (512 * 1024)
if args.fixture:
    fixture_source = args.fixture.resolve(strict=True)
    if not fixture_source.is_relative_to((HERE / 'runs').resolve()):
        parser.error('fixture must come from this probe synthetic runs')
    fixture_data = fixture_source.read_bytes()
    if len(fixture_data) != 512 * 1024:
        parser.error('fixture must be 512KiB')
with save.open('xb') as f:
    f.write(fixture_data)
marker = run / 'unrelated-marker.txt'
marker.write_text('Native app probe must preserve unrelated files.\n')
before = {'rom_sha256': digest(rom), 'eboot_sha256': digest(eboot), 'fixture_sha256': digest(save), 'marker_sha256': digest(marker)}
(run / 'before.json').write_text(json.dumps(before, indent=2) + '\n')
command = [str(emulator), *(['-i'] if os.environ.get('PSP_NATIVE_INTERP') else []), '--memstick=' + str(run / 'memstick'), '--timeout=' + str(args.seconds), '--log', '--loglevel=4', '--graphics=software', '--screenshot-save=' + str(run / 'screen.png'), str(stage / 'EBOOT.PBP')]
with (run / 'run.log').open('w') as log:
    try:
        result = subprocess.run(command, stdout=log, stderr=log, timeout=args.seconds + 30)
        status = result.returncode
    except subprocess.TimeoutExpired:
        status = 'host timeout'
after = {'returncode': status, 'rom_sha256': digest(rom), 'fixture_sha256': digest(save), 'fixture_bytes': save.stat().st_size, 'marker_sha256': digest(marker)}
(run / 'after.json').write_text(json.dumps(after, indent=2) + '\n')
print(run)
if after['rom_sha256'] != before['rom_sha256'] or after['marker_sha256'] != before['marker_sha256']:
    raise SystemExit('FAIL: ROM or unrelated marker changed')
if after['fixture_bytes'] != 512 * 1024:
    raise SystemExit('FAIL: native save fixture changed size')
print('ROM and unrelated marker preserved; synthetic save retained for inspection.')
print('Execution return code:', status, '(does not by itself establish game boot)')
for line in (run / 'run.log').read_text(errors='replace').splitlines():
    if 'stdout:' in line:
        print(line)
