from pathlib import Path
import os,re,shutil,subprocess,json
h=Path(__file__).resolve().parent;p=h/'title-proof';r=h/'title-render-profile';libs=p/'frozen-libraries';libs.mkdir(exist_ok=True)
env=os.environ.copy();env['PSPDEV']=str(Path.home()/'pspdev');env['PATH']=env['PSPDEV']+'/bin:'+env['PATH']
make=(p/'Makefile').read_text()
for i,lib in enumerate(sorted(set(re.findall(r'\S+\.a',make)))):
 if str(r) in lib:continue
 src=Path(lib).resolve();dst=src if src.parent==libs else libs/(str(i)+'-'+src.name)
 if src!=dst:shutil.copyfile(src,dst)
 make=make.replace(lib,str(dst))
(p/'Makefile').write_text(make)
exe='@PPSSPP@'
stage=p/'memstick/PSP/GAME/NativeSoulSilver';fixture=(stage/'SoulSilver.native.sav').read_bytes();result={}
original=(r/'texture_cache.original.h').read_text();candidate=(r/'texture_cache.h').read_text();assert 'SS_TextureBytesEqual' in candidate
for variant,source in [('baseline',original),('equal32',candidate)]:
 (r/'texture_cache.h').write_text(source)
 with (p/('texture-'+variant+'-build.log')).open('w') as f:
  subprocess.run(['make','-j4'],cwd=r,env=env,stdout=f,stderr=f,check=True)
  for name in ['ss-native-main.elf','ss-native-main.prx','EBOOT.PBP']:(p/name).unlink(missing_ok=True)
  subprocess.run(['make','-j4'],cwd=p,env=env,stdout=f,stderr=f,check=True)
 shutil.copyfile(p/'EBOOT.PBP',stage/'EBOOT.PBP');(stage/'SoulSilver.native.sav').write_bytes(fixture)
 log=p/('texture-'+variant+'.log');screen=p/('texture-'+variant+'.png')
 with log.open('w') as f:subprocess.run([exe,'--memstick='+str(p/'memstick'),'--timeout=20','--log','--loglevel=4','--graphics=software','--screenshot-save='+str(screen),str(stage/'EBOOT.PBP')],stdout=f,stderr=f,timeout=45,check=True)
 text=log.read_text();assert 'clean bounded probe exit' in text and 'sceKernelExitGame' in text and 'updates=1200' in text
 profile=[dict(zip(['frame','draw','bind','calls'],map(int,row))) for row in re.findall(r'\[SS-G3-STAGE\] frame=(\d+) draw=(\d+) bind=(\d+) calls=(\d+)',text)]
 result[variant]={'profile':profile,'steady_average':{k:sum(x[k] for x in profile if x['frame']>=600)/len([x for x in profile if x['frame']>=600]) for k in ['draw','bind','calls']},'elapsed_us':int(re.search(r'updates=1200 elapsed_us=(\d+)',text).group(1))}
(r/'texture_cache.h').write_text(candidate)
(h/'title-texture-ab.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v['steady_average'] for k,v in result.items()},indent=2))
with (h/'title-texture-pixels.json').open('w') as f:subprocess.run(['python3',str(h/'romfs-shared/compare_png.py'),str(p/'texture-baseline.png'),str(p/'texture-equal32.png')],stdout=f,check=True)
