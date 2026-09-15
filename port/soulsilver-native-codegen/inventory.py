from pathlib import Path
import json,re,collections,subprocess,os
import translate as t
HERE=t.HERE
rows=[]
for p in sorted(t.SRC.glob('*.s')):
 for name,body in re.findall(r'\bthumb_func_start\s+(\w+)\s*\n(.*?)\bthumb_func_end\s+\w+',p.read_text(),re.S):
  try:
   code,ops=t.emit(name,body);accepted=True;reason='candidate; ABI/layout not yet audited'
  except (ValueError,IndexError) as e:accepted=False;reason=str(e)
  rows.append(dict(name=name,file=p.name,accepted=accepted,reason=reason))
graph=json.loads((HERE.parent/'soulsilver-native-core/graph-NitroMain.json').read_text())
need={x['symbol'] for x in graph['unresolved']}
for row in rows:row['NitroMain_dependency']=row['name'] in need
(HERE/'support-inventory.json').write_text(json.dumps(rows,indent=2)+'\n')
print('total',len(rows),'accepted ISA candidates',sum(x['accepted'] for x in rows),'NitroMain candidate count',sum(x['accepted'] and x['NitroMain_dependency'] for x in rows))
print('accepted unresolved:',[x['name'] for x in rows if x['accepted'] and x['NitroMain_dependency']])
print(collections.Counter(x['reason'] for x in rows if not x['accepted']).most_common(12))
