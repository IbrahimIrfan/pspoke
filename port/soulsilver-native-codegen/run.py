from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent
pspdev=Path(os.environ.get('PSPDEV',Path.home()/'pspdev'))
env=dict(os.environ,PSPDEV=str(pspdev),PATH=str(pspdev/'bin')+':'+os.environ['PATH'])
subprocess.run([sys.executable,'translate.py'],cwd=here,check=True)
with (here/'build.log').open('w') as f:subprocess.run(['make','-B','-j4'],cwd=here,env=env,stdout=f,stderr=f,check=True)
exe=Path(os.environ.get('PPSSPP_HEADLESS','@PPSSPP@'))
with (here/'run.log').open('w') as f:r=subprocess.run([str(exe),'--memstick='+str(here/'memstick'),'--timeout=10','--log','--loglevel=4','--graphics=software',str(here/'EBOOT.PBP')],stdout=f,stderr=f,timeout=40,check=True)
log=(here/'run.log').read_text()
for line in log.splitlines():
 if 'CODEGEN' in line: print(line)
assert 'failures=0 hash=e5e8c81c' in log
assert 'same_hash=1 hash=7fc746f8' in log
assert 'I sceKernel: sceKernelExitGame' in log
print('PASS: generated native components match independent references; clean PSP exit.')
