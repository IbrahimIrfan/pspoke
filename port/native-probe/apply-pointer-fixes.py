from pathlib import Path
import difflib
b=Path(__file__).resolve().parent;r=b/'pokeplatinum';patch=[]
# Only pointer-table layout/initializers use DS-width branch; all platform/OS behavior stays SDK_PORT.
regions={'src/underground/player_talk.c':(210,240),'src/underground/menus.c':(155,190),'src/overlay076/ov76_0223D338.c':(215,238),'src/applications/bag/main.c':(320,360),'include/overlay076/struct_ov76_0223BF74.h':(1,20)}
for name,(lo,hi) in regions.items():
 p=r/name;old=p.read_text();lines=old.splitlines(keepends=True)
 for i in range(lo-1,min(hi,len(lines))):
  if '#ifdef SDK_BUILD_ARM' in lines[i]:lines[i]=lines[i].replace('#ifdef SDK_BUILD_ARM','#if defined(SDK_BUILD_ARM) || defined(SDK_BUILD_PSP)')
 new=''.join(lines);patch.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+name,tofile='b/'+name));p.write_text(new)
(b/'psp-pointer-tables.patch').write_text(''.join(patch))
