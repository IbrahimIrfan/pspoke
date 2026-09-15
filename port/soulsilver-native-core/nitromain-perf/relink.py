"""Relink the strict diagnostic after actual-code/data archives change.
Only known original function entry points receive fatal traps. No data placeholders.
"""
from pathlib import Path
import os,subprocess
b=Path(__file__).resolve().parent;env=os.environ.copy();env['PSPDEV']=str(Path.home()/'pspdev');env['PATH']=env['PSPDEV']+'/bin:'+env['PATH']
(b/'EBOOT.PBP').unlink(missing_ok=True)
(b/'ss-native-main.prx').unlink(missing_ok=True)
p=b/'Makefile';normal=p.read_text();without=normal.replace('missing.o ','');p.write_text(without)
try:
 (b/'ss-native-main.elf').unlink(missing_ok=True)
 with (b/'build.log').open('w') as log:subprocess.run(['make','-j4'],cwd=b,env=env,stdout=log,stderr=log)
 with (b/'trap-generation.txt').open('w') as log:subprocess.run(['python3','generate_traps.py'],cwd=b,check=True,stdout=log)
finally:p.write_text(normal if 'missing.o ' in normal else normal.replace('OBJS = ','OBJS = missing.o '))
(b/'missing.o').unlink(missing_ok=True);(b/'ss-native-main.elf').unlink(missing_ok=True)
with (b/'build-with-traps.log').open('w') as log:r=subprocess.run(['make','-j4'],cwd=b,env=env,stdout=log,stderr=log)
print((b/'trap-generation.txt').read_text());print('Diagnostic link status',r.returncode)
if r.returncode:raise SystemExit(r.returncode)

subprocess.run(['python3','build_manifest.py'],cwd=b,check=True)
