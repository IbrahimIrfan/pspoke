from pathlib import Path
import re,shutil
b=Path(__file__).resolve().parent;root=b.parent/'soulsilver-research/pokeheartgold-slop'
text='\n'.join(p.read_text(errors='replace') for d in [root/'src',root/'include'] for p in d.rglob('*') if p.suffix in ['.c','.h'])
for p in (b/'include').rglob('*.naix'):
 prefix=p.stem; symbols={}
 for sym in set(re.findall(r'\bNARC_'+re.escape(prefix)+r'_\w+',text)):
  suffix=sym[len('NARC_'+prefix+'_'):];m=re.search(r'(?:^|_)(\d{1,8})(?:_|$)',suffix)
  if m:symbols[sym]=int(m.group(1))
 if symbols:p.write_text('#pragma once\n/* Numbered original asset IDs; named manifests override below. */\n'+''.join(f'#define {k} {v}\n' for k,v in sorted(symbols.items())))
 # Hidden original archive order overrides heuristic numeric filenames.
 assetdir=root/'files'/p.relative_to(b/'include').with_suffix('')
 order=assetdir/'.narcorder'
 if order.exists():
  names=[s.strip() for s in order.read_text().splitlines() if s.strip() and not s.lstrip().startswith('#')]
  with p.open('a') as f:
   for i,n in enumerate(names):f.write(f'#define NARC_{prefix}_{re.sub("[^a-zA-Z0-9_]","_",Path(n).name)} {i}\n')
 # Independently verified ROM-backed exact map.
 verified=b.parent/'soulsilver-native-assets/member-maps'/p.name
 if verified.exists():shutil.copyfile(verified,p)
# Item header directory differs from NARC basename.
p=b/'include/itemtool/itemdata/item_data.naix';p.parent.mkdir(parents=True,exist_ok=True)
p.write_text('#pragma once\n'+''.join(f'#define {s} {int(re.search(r"NARC_item_data_(\d+)_bin",s)[1])}\n' for s in sorted(set(re.findall(r'NARC_item_data_\d+_bin',text)))))
# This public source includes the header with an unusual relative source-root path.
p=b/'crossprobe.py';s=p.read_text().replace("base/'game-include/library'", "base/'game-include/library',base/'src'");p.write_text(s)
