from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent;probe=here/'library-proof'
subprocess.run([sys.executable,str(here/'library.py')],check=True)
pspdev=Path(os.environ.get('PSPDEV',Path.home()/'pspdev'))
env=dict(os.environ,PSPDEV=str(pspdev),PATH=str(pspdev/'bin')+':'+os.environ['PATH'])
with (probe/'build.log').open('w') as f:subprocess.run(['make','-B','-j4'],cwd=probe,env=env,stdout=f,stderr=f,check=True)
exe=Path(os.environ.get('PPSSPP_HEADLESS','@PPSSPP@'))
with (probe/'run.log').open('w') as f:subprocess.run([str(exe),'--memstick='+str(probe/'memstick'),'--timeout=10','--log','--loglevel=4','--graphics=software',str(probe/'EBOOT.PBP')],stdout=f,stderr=f,timeout=40,check=True)
log=(probe/'run.log').read_text()
for line in log.splitlines():
 if 'LIBRARY' in line:print(line)
assert 'checks=1441933 errors=0 final_seed=db586520' in log
assert 'I sceKernel: sceKernelExitGame' in log
print('PASS: typed native library and source-based SDK call reference agree.')
