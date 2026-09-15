"""Stage a native application test without touching actual saves or the SD card."""
from pathlib import Path
import argparse, datetime, hashlib, json, os, shutil, subprocess

HERE = Path(__file__).resolve().parent
DEFAULT_ROM = HERE.parents[2] / 'Pokemon - Platinum Version (USA) (Rev 1).nds'
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
parser.add_argument('--seconds', type=int, default=8)
parser.add_argument('--fixture', type=Path, help='Existing synthetic fixture under this probe runs directory; copied, never modified')
args = parser.parse_args()
rom, emulator = args.rom.resolve(strict=True), args.ppsspp.resolve(strict=True)
eboot = (HERE / 'EBOOT.PBP').resolve(strict=True)
if not 1 <= args.seconds <= 300:
    parser.error('--seconds must be 1..300 wall-clock seconds')
# New directory and exclusive save creation: existing game saves are never read.
run = HERE / 'runs' / datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
stage = run / 'memstick/PSP/GAME/NativePlatinum'
stage.mkdir(parents=True, exist_ok=False)
shutil.copyfile(eboot, stage / 'EBOOT.PBP')
(stage / 'Platinum.nds').symlink_to(rom)
save = stage / 'Platinum.native.sav'
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
command = [str(emulator), '--memstick=' + str(run / 'memstick'), '--timeout=' + str(args.seconds), '--log', '--loglevel=4', '--graphics=software', '--screenshot-save=' + str(run / 'screen.png'), str(stage / 'EBOOT.PBP')]
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
