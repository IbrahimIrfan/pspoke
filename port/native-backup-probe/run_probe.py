from pathlib import Path
import subprocess,os,time
base=Path(__file__).resolve().parent
stage=base/'runs'/str(time.time_ns())/'memstick/PSP/GAME/NativeBackup';stage.mkdir(parents=True)
env=dict(os.environ,PSPDEV='@PSPDEV@');env['PATH']=env['PSPDEV']+'/bin:'+env['PATH']
with (base/'build.log').open('w') as log:subprocess.run(['make','-C',str(base)],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
(stage/'EBOOT.PBP').write_bytes((base/'EBOOT.PBP').read_bytes())
with (stage/'probe-save.bin').open('xb') as file:file.write(bytes([255])*524288)
(stage/'untouched.txt').write_text('other data must remain unchanged\n')
headless='@PPSSPP@'
with (base/'run.log').open('w') as log:subprocess.run([headless,'--memstick='+str(stage.parents[2]),'--timeout=6','--log','--loglevel=4','--graphics=software',str(stage/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT,check=True)
log=(base/'run.log').read_text();assert '[BACKUP] failures=0 callbacks=4 persistent777=checked' in log
expected=bytearray([255])*524288;expected[255:1032]=bytes((i*37+11)&255 for i in range(777))
assert (stage/'probe-save.bin').read_bytes()==expected
assert (stage/'untouched.txt').read_text()=='other data must remain unchanged\n'
assert not (stage/'does-not-exist.bin').exists()
(base/'last-stage.txt').write_text(str(stage)+'\n')
print('PSP backup probe zero failures; full synthetic file checked; unrelated data intact.')
