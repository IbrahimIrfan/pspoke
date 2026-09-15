from pathlib import Path
import os,subprocess,sys,argparse,json
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('proof',choices=['player-control-proof','player-direction-proof','touch-proof','mi-clear-proof','account-proof','sound-flag-proof','sound-count-proof','comm-proof','sound-scene-proof','sound-gb-proof','sound-move-proof']);args=parser.parse_args();probe=here/args.proof
subprocess.run([sys.executable,str(here/'library.py')],check=True)
if (probe/'abi.c').exists():
 cmd=json.loads((here.parent/'soulsilver-native-core/compile-command.json').read_text())
 with (probe/'abi.log').open('w') as f:subprocess.run(cmd+['-c',str(probe/'abi.c'),'-o',str(probe/'abi.o')],stdout=f,stderr=f,check=True)
pspdev=Path(os.environ.get('PSPDEV',Path.home()/'pspdev'));env=dict(os.environ,PSPDEV=str(pspdev),PATH=str(pspdev/'bin')+':'+os.environ['PATH'])
with (probe/'build.log').open('w') as f:subprocess.run(['make','-B','-j4'],cwd=probe,env=env,stdout=f,stderr=f,check=True)
exe=Path(os.environ.get('PPSSPP_HEADLESS','@PPSSPP@'))
with (probe/'run.log').open('w') as f:subprocess.run([str(exe),'--memstick='+str(probe/'memstick'),'--timeout=15','--log','--loglevel=4','--graphics=software',str(probe/'EBOOT.PBP')],stdout=f,stderr=f,timeout=40,check=True)
log=(probe/'run.log').read_text();wanted={'player-control-proof':'cases=49152 checks=34701312 errors=0','player-direction-proof':'scalar_cases=65536 priority_cases=55296 checks=7319552 errors=0','touch-proof':'cases=4096 byte_checks=409600 errors=0','mi-clear-proof':'cases=17921 byte_checks=151388160 errors=0','account-proof':'cases=4096 checks=335872 errors=0 final_seed=0d981678','sound-flag-proof':'cases=65536 checks=2228224 errors=0 final_seed=36b95678','sound-count-proof':'cases=65536 checks=262144 errors=0 final_seed=36b95678','comm-proof':'cases=10240 checks=21088256 errors=0 final_seed=aca91e78','sound-scene-proof':'cases=19456 checks=22452224 errors=0 final_seed=6b0f7704','sound-gb-proof':'map_cases=65536 state_channel_cases=4096 volume_cases=65536 getter_cases=65536 checks=11422720 errors=0 final_seed=5ab24678','sound-move-proof':'cases=131072 checks=4390912 errors=0 final_seed=7fc35678'}[args.proof]
for line in log.splitlines():
 if wanted in line:print(line)
assert wanted in log and 'I sceKernel: sceKernelExitGame' in log
print('PASS:',args.proof)
