"""Compare emitted numeric table bytes against the user's ROM; relocations excluded."""
from pathlib import Path
import sys,struct,re,json
BASE=Path(__file__).resolve().parent;sys.path.insert(0,str(BASE/'python'))
import ndspy.rom
SRC=BASE.parent/'soulsilver-research/pokeheartgold-slop'
rom=ndspy.rom.NintendoDSRom.fromFile(Path('@SOULSILVER_ROM@'))
main=rom.loadArm9();overlays=rom.loadArm9Overlays();owner={};current=None;ov=0
for line in (SRC/'main.lsf').read_text().splitlines():
 m=re.match(r'^(Static|Autoload|Overlay)\s+(\w+)',line)
 if m:
  current=ov if m[1]=='Overlay' else None
  if m[1]=='Overlay':ov+=1
 m=re.match(r'\s*Object asm/(\S+)\.o',line)
 if m:owner[m[1]]=current

def elf(path):
 b=path.read_bytes();shoff=struct.unpack_from('<I',b,32)[0];count,strings=struct.unpack_from('<HH',b,48);sections=[struct.unpack_from('<10I',b,shoff+i*40) for i in range(count)]
 def content(i):s=sections[i];return b[s[4]:s[4]+s[5]]
 sym=[];rel={}
 for i,s in enumerate(sections):
  if s[1]==2:
   names=content(s[6])
   for off in range(0,s[5],s[9]):
    name,value,size,info,other,index=struct.unpack_from('<IIIBBH',b,s[4]+off)
    if name:sym.append((names[name:names.index(0,name)].decode(),value,index))
  elif s[1]==9:
   covered=rel.setdefault(s[7],set())
   for off in range(0,s[5],s[9]):
    pos,info=struct.unpack_from('<II',b,s[4]+off);width=4 if (info&255)==2 else 2
    covered.update(range(pos,pos+width))
 return sections,content,sym,rel

def original(stem,address,size):
 if stem not in owner:return None
 index=owner[stem]
 regions=[(s.ramAddress,s.data) for s in main.sections] if index is None else [(overlays[index].ramAddress,overlays[index].data)]
 for base,data in regions:
  if base<=address and address+size<=base+len(data):return data[address-base:address-base+size]
 return None
results=[]
adjustments=[]
# Numeric address suffixes are original disassembly labels, not an ELF map.
# Infer a section offset only with >=3 unique, independent ROM byte anchors.
def section_offsets(stem,sections,content,syms,rel,reverse):
 if stem not in owner or owner[stem] is None:return {}
 ov=overlays[owner[stem]];votes={}
 for name,value,index in syms:
  m=re.search(r'(0[12][0-9a-fA-F]{6})$',reverse[name])
  if not m or sections[index][1]!=1:continue
  nexts=[v for n,v,i in syms if i==index and v>value]
  size=min(32,(min(nexts) if nexts else sections[index][5])-value)
  if size<16 or any(x in rel.get(index,set()) for x in range(value,value+size)):continue
  b=content(index)[value:value+size]
  if len(set(b))<5:continue
  at=ov.data.find(b)
  if at<0 or ov.data.find(b,at+1)>=0:continue
  delta=ov.ramAddress+at-int(m[1],16)
  votes.setdefault(index,{}).setdefault(delta,[]).append(reverse[name])
 offsets={}
 for index,vs in votes.items():
  delta,anchors=max(vs.items(),key=lambda x:len(x[1]))
  if len(anchors)>=3:
   offsets[index]=delta
   if delta:adjustments.append({'file':stem,'section':index,'delta':delta,'anchors':anchors})
 return offsets
for item in json.loads((BASE/'inventory.json').read_text()):
 if item['status']!='pass':continue
 stem=Path(item['file']).stem;sections,content,syms,rel=elf(BASE/'objects'/(stem+'.o'));reverse={v:k for k,v in item['symbols'].items()}
 syms=[(name,value,index) for name,value,index in syms if name in reverse and index<len(sections)]
 offsets=section_offsets(stem,sections,content,syms,rel,reverse)
 source=(SRC/item['file']).read_text()
 for name,value,index in syms:
  originalname=reverse[name];m=re.search(r'(0[12][0-9a-fA-F]{6})$',originalname)
  if not m or sections[index][1]!=1:continue
  ends=[v for n,v,i in syms if i==index and v>value];end=min(ends) if ends else sections[index][5]
  data=content(index)[value:end]
  address=int(m[1],16)+offsets.get(index,0)
  comment=re.search(r'^'+re.escape(originalname)+r':\s*;\s*(0x[0-9A-Fa-f]+)',source,re.M)
  if comment and int(comment[1],16)!=int(m[1],16):
   address=int(comment[1],16)+offsets.get(index,0)
   adjustments.append({'file':stem,'symbol':originalname,'comment_address':comment[1]})
  expected=original(stem,address,len(data))
  if expected is None:continue
  excluded=rel.get(index,set());positions=[i for i in range(len(data)) if value+i not in excluded]
  wrong=[i for i in positions if data[i]!=expected[i]]
  results.append({'file':stem,'symbol':originalname,'bytes':len(positions),'bad':len(wrong),'first_bad':wrong[:8]})
report={'regions':len(results),'bytes':sum(x['bytes'] for x in results),'bad_bytes':sum(x['bad'] for x in results),'bad_regions':[x for x in results if x['bad']],'address_adjustments':adjustments,'scope':'Numeric data only; address-labelled regions in linked ASM objects; native relocations excluded'}
(BASE/'rom-verify.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='bad_regions'},indent=2));print('badregions',report['bad_regions'][:8])
