from pathlib import Path
import os, subprocess, hashlib, json
base=Path(__file__).resolve().parent
rom=Path('@PLATINUM_ROM@')
ppsspp=Path('@PPSSPP@')
def sha256(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()
before=sha256(rom)
subprocess.run(['python3',str(base/'host_check.py')],check=True)
env=dict(os.environ,PSPDEV='@PSPDEV@')
env['PATH']=env['PSPDEV']+'/bin:'+env['PATH']
with (base/'build.log').open('w') as log:
 subprocess.run(['make','clean'],cwd=base,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run(['make','-j4'],cwd=base,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
stage=base/'memstick/PSP/GAME/NativeRomFS';stage.mkdir(parents=True,exist_ok=True)
(stage/'EBOOT.PBP').write_bytes((base/'EBOOT.PBP').read_bytes())
link=stage/'Platinum.nds'
if not link.is_symlink():
 if link.exists():raise RuntimeError('Refusing to replace existing staged ROM')
 link.symlink_to(rom)
assert link.resolve()==rom
with (base/'run.log').open('w') as log:
 subprocess.run([str(ppsspp),'--memstick='+str(base/'memstick'),'--timeout=5','--log','--loglevel=4','--graphics=software',str(stage/'EBOOT.PBP')],cwd=base,stdout=log,stderr=subprocess.STDOUT,check=True)
after=sha256(rom)
assert before==after,'ROM changed'
log=(base/'run.log').read_text()
assert '[ROMFS] cases=30 failures=0' in log and '[ROMFS] actual narc.c member cases=18' in log
(base/'integrity.json').write_text(json.dumps(dict(sha256_before=before,sha256_after=after,unchanged=before==after),indent=2)+'\n')
print('\n'.join(line for line in log.splitlines() if '[ROMFS]' in line))
print('ROM SHA-256 unchanged; no extracted assets or save writes.')
