# Reproduce metadata-only portions of upstream dexproc.c and nitrosfx/sdat.c.
# Does not fabricate archive members or access audio/image assets.
from pathlib import Path
import json,re,collections
b=Path(__file__).resolve().parent;g=b/'generated';r=b/'pokeplatinum'
p=g/'res/pokemon';p.mkdir(parents=True,exist_ok=True)
(p/'regional_pokedex_size.h').write_text('#define REGIONAL_DEX_COUNT %d\n'%(len(json.loads((r/'res/pokemon/sinnoh_pokedex.json').read_text()))-1))
out=['#ifndef PSP_PROBE_PL_SOUND_NAIX','#define PSP_PROBE_PL_SOUND_NAIX']
for key,entries in json.loads((r/'res/sound/pl_sound_data.json').read_text()).items():
 if not isinstance(entries,list):continue
 seen=collections.Counter()
 for idx,e in enumerate(entries):
  if not e.get('name'):continue
  filename=e.get('fileName',e['name']);nhits=seen[filename];seen[filename]+=1
  guard=re.sub('[^A-Za-z0-9_]','_',filename)+(('_'+str(nhits)) if nhits else '')
  out.append(f'#define {guard} {idx}')
out.append('#endif');p=g/'res/sound';p.mkdir(parents=True,exist_ok=True);(p/'pl_sound_data.naix').write_text('\n'.join(out)+'\n')
print('Sound declarations',len(out)-3)
# nitroarc's header-only declaration projection from exact ordered member names.
orders=list(r.glob('res/**/*.order'))
for src in orders:
 members=[x.strip() for x in src.read_text().splitlines() if x.strip() and not x.lstrip().startswith('#')]
 dest=(g/src.relative_to(r)).with_suffix('.naix');dest.parent.mkdir(parents=True,exist_ok=True)
 guard=re.sub('[^A-Za-z0-9_]','_',str(dest.relative_to(g)));lines=[f'#ifndef {guard}',f'#define {guard}'];seen=collections.Counter()
 for i,member in enumerate(members):
  n=seen[member];seen[member]+=1;name=re.sub('[^A-Za-z0-9_]','_',member)+(('_'+str(n)) if n else '')
  lines.append(f'#define {name} {i}')
 lines += [f'#define {guard[:-5]}_COUNT {len(members)}','#endif'];dest.write_text('\n'.join(lines)+'\n')
print('Archive manifests projected',len(orders))
# Aliases established by each directory's Meson custom_target output and order input.
aliases={'res/pokemon/species_icons':'pl_poke_icon','res/field/scripts/scripts':'scr_seq','res/field/lighting/lighting_sets':'lighting','res/field/encounters/encounters':'pl_enc_data','res/field/matrices/map_matrices':'map_matrix','res/field/maps/texture_sets/map_texture_sets':'map_tex_set','res/field/props/models/map_prop_models':'prop_models','res/graphics/naming_screen/naming_screen':'namein','res/graphics/pokedex/pokedex':'zukan','res/graphics/poffin_case/poffin_case':'poru_gra','res/graphics/pokemon_summary_screen/pokemon_summary_screen':'pl_pst_gra'}
for src,name in aliases.items():
 p=(g/src).with_suffix('.naix')
 if p.exists():p.with_name(name+'.naix').write_text(p.read_text())
# msgenc Json::WriteHeader: ID order is message-array order; text comments omitted.
n=0
for src in (r/'res/text').glob('*.json'):
 data=json.loads(src.read_text());messages=data.get('messages')
 if messages is None:continue
 rel=Path('res/text/bank')/(src.stem+'.h');guard=re.sub('[/.-]','_',str(rel)).upper();lines=[f'#ifndef MSGENC_{guard}',f'#define MSGENC_{guard}']
 for i,msg in enumerate(messages):lines.append(f"#define {msg['id']} {i}")
 lines += [f'#define {guard[4:-2]}_ENTRY_COUNT {len(messages)}','#endif'];dest=g/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text('\n'.join(lines)+'\n');n+=1
print('Text bank header projections',n)
# moveproc.c proc_text identifies messages by generated move-enum order.
moves=[x.strip()[5:].lower() for x in (r/'generated/moves.txt').read_text().splitlines() if x.startswith('MOVE_')]
for bank,prefix in [('move_names','MoveNames_Text_'),('move_names_uppercase','MoveNamesUppercase_Text_'),('move_descriptions','MoveDescriptions_Text_')]:
 dest=g/'res/text/bank'/f'{bank}.h';guard='TEXT_BANK_'+bank.upper()+'_ENTRY_COUNT';dest.write_text('\n'.join(f'#define {prefix}{name} {i}' for i,name in enumerate(moves))+f'\n#define {guard} {len(moves)}\n')
# Actual subscript/end-credit manifest headers are emitted one directory above or included by short name.
for src,dest in [('res/battle/scripts/subscripts/sub_seq.naix','res/battle/scripts/sub_seq.naix'),('res/graphics/end_credits/ending.naix','ending.naix')]:
 p=g/src
 if p.exists():(g/dest).write_text(p.read_text())
# speciesproc.c emit_textbanks emits names in generated species-enum order.
species=[x.strip()[8:].lower() for x in (r/'generated/species.txt').read_text().splitlines() if x.startswith('SPECIES_')]
for bank in ['species_name','species_name_with_articles']:
 (g/'res/text/bank'/f'{bank}.h').write_text('\n'.join(f'#define {bank}_{name} {i}' for i,name in enumerate(species))+f'\n#define TEXT_BANK_{bank.upper()}_ENTRY_COUNT {len(species)}\n')
# resdatproc.c dump_header exports only templates-array IDs.
for src in (r/'res/graphics/sprite_templates').glob('*.json'):
 data=json.loads(src.read_text());templates=data.get('templates',[]);dest=g/'res/graphics/sprite_templates'/(src.stem+'.h');dest.parent.mkdir(parents=True,exist_ok=True)
 dest.write_text('\n'.join(f"#define {entry['id']} {i}" for i,entry in enumerate(templates))+'\n')
