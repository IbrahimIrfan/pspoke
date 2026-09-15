from pathlib import Path
import subprocess,os,shutil,sys
b=Path(__file__).resolve().parent;d=b.parent/'native-lake-render';env=dict(os.environ,PSPDEV='@PSPDEV@',PATH='@PSPDEV@/bin:'+os.environ['PATH'])
for n in ['g3_handler.cpp','g3_backend.cpp']:os.utime(d/n,None)
with(d/'build.log').open('w')as log:subprocess.run(['make','-j4','EXTRA_CFLAGS=-DPSP_NATIVE_GE_TRANSFORM=1 '+(' '.join(sys.argv[2:]))],cwd=d,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
for name in ['libnative-render-frozen.a','libnative-render-gu2d.a']:shutil.copy2(d/'libnative-render-gu2d.a',b/name)
(b/'native-app.elf').unlink(missing_ok=True);cmd=['make','-j4']
for p in b.glob('*.o'):cmd+=['-o',p.name]
with(b/'build.log').open('w')as log:subprocess.run(cmd,cwd=b,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
with(b/(sys.argv[1]+'-summary.log')).open('w')as log:subprocess.run(['python3','run_probe.py','--seconds','30','--fixture','runs/verified-lake/Platinum.native.sav'],cwd=b,stdout=log,stderr=subprocess.STDOUT,check=True)
