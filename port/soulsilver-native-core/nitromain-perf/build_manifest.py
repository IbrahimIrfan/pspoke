from pathlib import Path
import hashlib,json,re
b=Path(__file__).resolve().parent;m=(b/'Makefile').read_text();paths={b/'Makefile',b/'linkfile.prx',b/'EBOOT.PBP',b/'ss-native-main.prx',b/'ss-native-main.elf'}
for line in m.splitlines():
 if line.startswith('OBJS =') or line.startswith('LIBS ='):
  for token in line.split():
   if token.endswith(('.o','.a')):paths.add((b/token).resolve())
for glob in ['*.c','*.py']:
 paths.update(b.glob(glob))
entries=[]
for p in sorted(paths):
 if p.is_file():entries.append({'path':str(p),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(b/'build-manifest.json').write_text(json.dumps({'scope':'native diagnostic build inputs; sibling sources separately documented','files':entries},indent=2)+'\n')
