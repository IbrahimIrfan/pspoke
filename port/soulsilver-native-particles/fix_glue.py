from pathlib import Path
import re,json,subprocess,difflib
b=Path(__file__).resolve().parent;c=b.parent/'soulsilver-native-core';old=(c/'src/unk_02014DA0.c').read_text();s=old
s=s.replace('u32 (*allocFn)(u32, BOOL)','void *(*allocFn)(u32)')
s=re.sub(r'static u32 (sub_020(?:14FA4|14FD0|14FFC|15028|15054|15080|150AC|150D8|15104|15130|1515C|15188|151B4|151E0|1520C|15238))\(u32 size, BOOL is4x4comp\)',r'static void *\1(u32 size)',s)
s=s.replace('static u32 (*const sFuncTable[16])(u32, BOOL)','static void *(*const sFuncTable[16])(u32)').replace('return (u32)old;','return old;')
s=s.replace('extern SPLEmitter *SPL_Create(SPLManager *mgr);','extern SPLEmitter *SPL_Create(SPLManager *mgr, int resourceID, const VecFx32 *pos);')
s=s.replace('void sub_02015484(SPLEmitter *emitter)', 'void sub_02015484(SPLEmitter *emitter, int resourceID, const VecFx32 *pos)').replace('SPL_Create(sys->spl);','SPL_Create(sys->spl, resourceID, pos);')
s=s.replace('void sub_020154C4(SPLEmitter *emitter)', 'void sub_020154C4(SPLEmitter *emitter, SPLEmitter *active)').replace('SPL_Delete(sys->spl, NULL);','SPL_Delete(sys->spl, active);')
(b/'glue.c').write_text(s);(b/'glue.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),s.splitlines(True),fromfile='src/unk_02014DA0.c',tofile='src/unk_02014DA0.c')))
cmd=json.loads((c/'compile-command.json').read_text())
with (b/'glue.log').open('w') as f:subprocess.run(cmd+['-c',str(b/'glue.c'),'-o',str(b/'glue.o')],stdout=f,stderr=f,check=True)
