from pathlib import Path
import re,json,struct
p=Path(__file__).resolve().parent;b=p.parent.parent/'native-probe';src=b/'pokeplatinum'
lsf=(src/'platinum.us/main.lsf').read_text()
modules=[]
for name,body in re.findall(r'Overlay\s+(\w+)\s*\{(.*?)\}',lsf,re.S):
 files=re.findall(r'Object main.nef.p/src_(\S+)\.c\.o',body)
 modules.append({'id':len(modules),'name':name,'objects':files,'other':[x.strip() for x in body.splitlines() if x.strip().startswith(('Object ','Library ')) and 'Object main.nef.p/src_' not in x]})
(p/'modules.json').write_text(json.dumps(modules,indent=2))
h=p/'include/nitro/fs';h.mkdir(parents=True,exist_ok=True)
s=(p.parent.parent/'native-graphics/libntr/include/nitro/fs/overlay.h').read_text().replace('#define\tFS_OVERLAY_ID(name)\t(0)','#include "native_overlay_ids.h"\n#define FS_OVERLAY_ID(name) (PSP_OVERLAY_ID_ ## name)')
(h/'overlay.h').write_text(s)
(h/'native_overlay_ids.h').write_text('\n'.join(f'#define PSP_OVERLAY_ID_{m["name"]} {m["id"]}u' for m in modules)+'\n')
# --headers-only: the SoulSilver build needs the overlay-id headers (shared services include nitro.h)
# but has no Platinum ROM; everything below reads the ROM.
import sys
if '--headers-only' in sys.argv: sys.exit(0)
# ROM is read only; only original segment bounds/file IDs retained as metadata.
rom=Path('@PLATINUM_ROM@')
with rom.open('rb') as f:
 hdr=f.read(512);off,size=struct.unpack_from('<II',hdr,0x50);f.seek(off);table=f.read(size)
assert len(table)==32*len(modules),(len(table),len(modules))
rows=[]
for i,m in enumerate(modules):
 vals=struct.unpack_from('<8I',table,i*32);assert vals[0]==i
 rows.append('{%s}'%','.join(f'0x{x:x}u' for x in vals[:4]))
(p/'rom_bounds.h').write_text('static const unsigned long rom_bounds[][4]={\n'+',\n'.join(rows)+'\n};\n')
print('modules',len(modules),'other records',[(m['name'],m['other']) for m in modules if m['other']])
