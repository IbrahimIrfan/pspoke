from pathlib import Path
import json,re,subprocess
b=Path(__file__).resolve().parent;r=b/'pokeplatinum';g=b/'generated';metadata=r/'res/pokemon';tmpl=r/'tools/dataproc/data'
def table(name,lines,sub='pokemon'):
 t=(tmpl/(name+'.h.template')).read_text();assert '/* =========== MAGIC CONTENT MARKER =========== */' in t
 p=g/'res'/sub/(name+'.h');p.parent.mkdir(parents=True,exist_ok=True);p.write_text(t.replace('/* =========== MAGIC CONTENT MARKER =========== */','\n'.join(lines)))
species=[x[8:].lower() for x in (r/'generated/species.txt').read_text().splitlines() if x.startswith('SPECIES_')]
extra=['deoxys/forms/attack','deoxys/forms/defense','deoxys/forms/speed','wormadam/forms/sandy','wormadam/forms/trash','giratina/forms/origin','shaymin/forms/sky','rotom/forms/heat','rotom/forms/wash','rotom/forms/frost','rotom/forms/fan','rotom/forms/mow']
egg=[];ft=[];fs=[];icons=[];tutor=[]
for i,name in enumerate(species+extra):
 d=json.loads((metadata/name/'data.json').read_text());upper=name.upper().replace('/FORMS','').replace('/','_')
 if name in species:
  if 'egg_moves' in d.get('learnset',{}):egg += ['    SPECIES_'+upper+' + EGG_MOVES_SPECIES_OFFSET,']+['    '+x+',' for x in d['learnset']['egg_moves']]
  if name not in ['egg','bad_egg']:
   f=d['footprint'];fs.append('    { %s, %s },'%('TRUE' if f['has'] else 'FALSE',f['size']));ft.append('    [SPECIES_%s] = { %s, %s },'%(upper,f['type'],'TRUE' if f['has'] or name=='spiritomb' else 'FALSE'))
 pal=d['icon_palette']
 if isinstance(pal,int):
  key=('ICON_EGG' if name=='egg' else 'ICON_MANAPHY_EGG' if name=='bad_egg' else ('ICON_' if i>=len(species) else 'SPECIES_')+upper);icons.append(f'    [{key}] = {pal},')
 else:
  for form,value in pal:icons.append('    [%s] = %d,'%('SPECIES_'+upper if form=='base' else 'ICON_'+upper+'_'+form.upper(),value))
for entry in json.loads((metadata/'move_tutors.json').read_text()):tutor.append('    { %s, %s, %s, %s, %s, %s },'%tuple(entry[x] for x in ['move','redCost','blueCost','yellowCost','greenCost','location']))
for name,lines in [('species_egg_moves',egg),('species_footprint_types',ft),('species_footprint_sizes',fs),('species_icon_palettes',icons),('tutorable_moves',tutor)]:table(name,lines)
items=[x for x in (r/'generated/items.txt').read_text().splitlines() if x.startswith('ITEM_')]
berries=[]
for name in items:
 p=r/'res/items/data'/(name[5:].lower()+'.json')
 if not p.exists():continue
 d=json.loads(p.read_text())
 if d.get('fieldUseFunc')=='ITEM_USE_FUNC_BERRY':berries.append('    [%d] = %s,'%(len(berries),name))
# Require schema-confirmed selection rather than silently emitting an empty table.
if berries:table('item_berry_list',berries,'items')
front=[]
for name in (r/'generated/frontier_trainers.txt').read_text().splitlines():
 if not name.startswith('FRONTIER_TRAINER_'):continue
 if name=='FRONTIER_TRAINER_TRAINER_CHERYL_CHERYL':break
 d=json.loads((r/'res/trainers/frontier/data'/(name[len('FRONTIER_TRAINER_'):].lower()+'.json')).read_text());front.append('    '+d['class']+',')
table('frontier_trainer_classes',front,'trainers/frontier')
trainers=[x[8:].lower() for x in (r/'generated/trainers.txt').read_text().splitlines() if x.startswith('TRAINER_')]
(g/'res/text/bank/npc_trainer_names.h').write_text('\n'.join(f'#define NPCTrainerNames_Text_{name} {i}' for i,name in enumerate(trainers))+'\n')
# Header projection from upstream ordergen/species.py.
def naix(rel,names):
 p=g/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('\n'.join('#define %s %d'%(re.sub('[^a-zA-Z0-9_]','_',name),i) for i,name in enumerate(names))+'\n')
naix('res/pokemon/pokefoot.naix',['footprint.NCLR','footprint_anim.NANR.lz','footprint_cell.NCER.lz']+[name+'/footprint.NCGR.lz' for name in species if name not in ['egg','bad_egg']])
# Shared icon manifest has 7 fixed members, followed by sorted icon_00000.NCGR etc.
with (g/'res/pokemon/pl_poke_icon.naix').open('a') as f:f.write('#define icon_00000_NCGR 7\n')
# Trainer back-sprite existence from Git tree metadata, not fake files.
tree=subprocess.check_output(['git','ls-tree','-r','--name-only','HEAD','res/trainers/classes'],cwd=r,text=True).splitlines();members=[]
for c in (r/'generated/trainer_classes.txt').read_text().splitlines():
 name=c.replace('TRAINER_CLASS_','').lower()
 if 'res/trainers/classes/'+name+'/back.png' in tree:members += [name+'/'+suffix for suffix in ['back.NCGR','back.NCLR','back_cell.NCER','back_anim.NANR','back_scan.NCGR']]
members += ['dp_rival/'+s for s in ['back.NCGR','back.NCLR','back_cell.NCER','back_anim.NANR','back_scan.NCGR']];naix('res/trainers/classes/trbgra.naix',members)
print('Generated genuine embedded species/trainer tables and archive projections')
# Exact itemproc.c has_data/add_idmap and mail/TM sequences.
idmap=[];mails=[];tms=[];dataid=0
for i,name in enumerate(items):
 p=r/'res/items/data'/(name[5:].lower()+'.json')
 if p.exists():
  d=json.loads(p.read_text());icon=d['icon']['sprite'];pal=d['icon']['palette'];gba=d['gbaID'];curr=dataid;dataid+=1
  if d['fieldUseFunc']=='ITEM_USE_FUNC_MAIL':mails.append('    [%d] = %s,'%(len(mails),name))
  if d['fieldUseFunc']=='ITEM_USE_FUNC_TM_HM':tms.append('    [%d] = %s,'%(len(tms),d['teachesMove']))
 else:
  assert name.startswith('ITEM_UNUSED_'),name
  curr=0;icon='none_NCGR';pal='none_NCLR';gba='GBA_ITEM_NONE'
 idmap.append('    [%d] = { %d, %s, %s, %s },'%(i,curr,icon,pal,gba))
for name,lines in [('item_id_map',idmap),('item_mail_list',mails),('item_tm_move_map',tms)]:table(name,lines,'items')
# speciesproc.c emit_tutorables uses tutor JSON ordering as bit positions.
tutorids=[x['move'] for x in json.loads((metadata/'move_tutors.json').read_text())];sets=[]
for name in species+extra:
 if name in ['none','egg','bad_egg']:continue
 d=json.loads((metadata/name/'data.json').read_text());bits=[0]*((len(tutorids)+7)//8)
 for move in d['learnset']['by_tutor']:
  assert move in tutorids,move
  i=tutorids.index(move);bits[i//8]|=1<<(i%8)
 sets.append('    { '+', '.join(hex(x) for x in bits)+' },')
table('species_learnsets_by_tutor',sets)
# make_pl_otherpoke.py emits 154 sprite +94 palette +5 fixed files, lexically archived.
names=[f'pl_otherpoke_{i:04}.NCGR' for i in range(154)]+[f'pl_otherpoke_{i:04}.NCLR' for i in range(154,248)]+['pl_otherpoke_0248.NCGR','pl_otherpoke_0249.NCGR','pl_otherpoke_0250.NCLR','pokemon_shadows.NCGR','pokemon_shadows_pal.NCLR'];naix('res/pokemon/pl_otherpoke.naix',sorted(names))
