from pathlib import Path
import shutil,re,xml.etree.ElementTree as ET
base=Path(__file__).resolve().parent
root=base.parent/'soulsilver-research/pokeheartgold-slop'
shutil.copytree(root/'include',base/'game-include',dirs_exist_ok=True)
p=base/'game-include/field/overlay_01_02204004.h'
s=p.read_text().replace('typedef struct NNSG3dMatAnmResult NNSG3dMatAnmResult;','').replace('typedef void (*NNSG3dFuncAnmMat)(NNSG3dMatAnmResult *, const NNSG3dAnmObj *, u32);','')
p.write_text(s)
shims={'nitro/os/ownerInfo.h':'#include <nitro/os/common/ownerInfo.h>\n'}
# owner SDK already declares the exact types under different include spelling.
for name,s in shims.items():
 p=base/'include'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s)
(base/'include/compat.h').write_text('''#include "global.h"
#define OS_IE_VBLANK OS_IE_V_BLANK
#define OS_IE_HBLANK OS_IE_H_BLANK
#define CARD_BACKUP_TYPE_FLASH_4MBITS_EX (CARDBackupType)(CARD_BACKUP_TYPE_FLASH_4MBITS | (255<<CARD_BACKUP_TYPE_VENDER_SHIFT))
''')
# Message IDs are explicit in public GMM XML; do not fabricate values.
for p in (root/'files/msgdata/msg').glob('*.gmm'):
 try: rows=ET.fromstring(p.read_text()).findall('row')
 except ET.ParseError: continue
 out=base/'include/msgdata/msg'/p.with_suffix('.h').name;out.parent.mkdir(parents=True,exist_ok=True)
 out.write_text('#pragma once\n'+''.join('#define '+r.attrib['id']+' '+r.attrib['index']+'\n' for r in rows))
# NARC assets encode their original explicit member index in their basename.
alltext='\n'.join(p.read_text(errors='replace') for p in list((root/'src').rglob('*.c'))+list((root/'include').rglob('*.h')))
includes=set(re.findall(r'#include ["<]([^">]+\.naix)[">]',alltext))
for name in includes:
 assetdir=root/'files'/name.removesuffix('.naix'); prefix=assetdir.name
 symbols={}
 for sym in set(re.findall(r'\bNARC_'+re.escape(prefix)+r'_\w+',alltext)):
  suffix=sym[len('NARC_'+prefix+'_'):]
  m=re.search(r'_(\d{4,8})(?:_|$)',suffix)
  if m: symbols[sym]=int(m.group(1))
 out=base/'include'/name;out.parent.mkdir(parents=True,exist_ok=True)
 out.write_text('#pragma once\n/* Member indices from public numbered asset identifiers. */\n'+''.join(f'#define {k} {v}\n' for k,v in sorted(symbols.items())))
print('Generated',len(includes),'NARC index headers')
# Local source copies permit declaration-only GCC compatibility fixes.
shutil.copytree(root/'src',base/'src',dirs_exist_ok=True)
for p in (base/'src').rglob('*.c'):
 s=p.read_text()
 # Public fork repeats SDK prototypes with void* placeholders; native SDK owns declarations.
 s=re.sub(r'^extern\s+[^;\n]+\b(?:NNS_\w+|NNSi_\w+|G2x_\w+|G3X_\w+|MATH_CountPopulation)\s*\([^;]*?;\s*$', '',s,flags=re.M)
 p.write_text(s)
p=base/'game-include/overlay_01_021EA824.h';p.write_text('\n'.join(l for l in p.read_text().splitlines() if not l.startswith('void G3X_SetFogTable('))+'\n')
p=base/'include/compat.h';p.write_text(p.read_text()+'''\n#define mapingType mappingType
#define ALIGN(n) __attribute__((aligned(n)))
#define GX_PLANEMASK_ALL (GX_PLANEMASK_BG0|GX_PLANEMASK_BG1|GX_PLANEMASK_BG2|GX_PLANEMASK_BG3|GX_PLANEMASK_OBJ)
''')
for name,target in {'nitro/os/cache.h':'nitro/os/ARM9/cache.h','nitro/hw/common/io_reg.h':'nitro/hw/ARM9/io_reg.h'}.items():
 p=base/'include'/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('#include <'+target+'>\n')
shutil.copyfile(root/'lib/include/dsprot.h',base/'include/dsprot.h')
