"""Read-only ROM asset/index checks against public .narcorder and data manifests."""
from pathlib import Path
import struct,json,re,hashlib
HERE=Path(__file__).resolve().parent
SRC=HERE.parent/'soulsilver-native-audit/pokeheartgold'
ROM=Path('@SOULSILVER_ROM@')
u16=lambda b,p=0:struct.unpack_from('<H',b,p)[0]
u32=lambda b,p=0:struct.unpack_from('<I',b,p)[0]
f=ROM.open('rb');h=f.read(512)
def read(off,n):f.seek(off);return f.read(n)
fnt=read(u32(h,0x40),u32(h,0x44));fat=read(u32(h,0x48),u32(h,0x4c));files={}
def walk(di,prefix):
 off=u32(fnt,di*8);idx=u16(fnt,di*8+4)
 while fnt[off]:
  tag=fnt[off];off+=1;name=fnt[off:off+(tag&127)].decode();off+=tag&127
  if tag&128:sub=u16(fnt,off);off+=2;walk(sub&4095,prefix+name+'/')
  else:files[prefix+name]=idx;idx+=1
walk(0,'')
def members(path):
 idx=files[path];a,b=struct.unpack_from('<II',fat,idx*8);data=read(a,b-a);assert data[:4]==b'NARC'
 blocks={};off=u16(data,12)
 while off<len(data):n=u32(data,off+4);assert n>=8;blocks[data[off:off+4]]=data[off:off+n];off+=n
 tab=blocks[b'BTAF'];payload=blocks[b'GMIF'][8:]
 return [payload[u32(tab,12+i*8):u32(tab,16+i*8)] for i in range(u16(tab,8))]
def unlz(b):
 if b[0]!=0x10:return b
 n=u32(b)>>8;out=bytearray();pos=4
 while len(out)<n:
  flag=b[pos];pos+=1
  for k in range(8):
   if len(out)>=n:break
   if flag&(128>>k):
    a,c=b[pos:pos+2];pos+=2;count=(a>>4)+3;dist=((a&15)<<8|c)+1;
    for _ in range(count):out.append(out[-dist])
   else:out.append(b[pos]);pos+=1
 return bytes(out[:n])
result={}; out=HERE/'member-maps';out.mkdir(exist_ok=True)
for namespace,folder,path in [('camera_viewfinder','graphic/camera_viewfinder','a/2/6/1'),('sbox_gra','data/sbox_gra','a/1/6/5'),('preview_graphic','fielddata/graphic/preview_graphic/preview_graphic','a/1/5/0')]:
 folder=SRC/'files'/folder;names=[x.strip() for x in (folder/'.narcorder').read_text().splitlines() if x.strip() and not x.startswith('#')];ms=members(path);assert len(ms)==len(names),(namespace,len(ms),len(names))
 verified=0
 for name,b in zip(names,ms):
  raw=unlz(b) if name.endswith('.lz') else b
  ext=name.removesuffix('.lz').split('.')[-1];assert raw[:4]=={'NCLR':b'RLCN','NCGR':b'RGCN','NSCR':b'RCSN'}[ext],(name,raw[:4])
  source=folder/name.removesuffix('.lz')
  if source.exists():
   sb=source.read_bytes();assert raw[:len(sb)]==sb,(name,len(sb),len(raw),[(i,x,y) for i,(x,y) in enumerate(zip(sb,raw)) if x!=y][:12]);verified+=1
 mapping={'NARC_'+namespace+'_'+x.replace('.','_'):i for i,x in enumerate(names)}
 (out/(namespace+'.naix')).write_text('#pragma once\n/* Verified public .narcorder, ROM member magic and available NSCR contents. */\n'+''.join(f'#define {k} {v}\n' for k,v in mapping.items()))
 result[namespace]={'rom_path':path,'members':len(names),'source_exact_checks':verified,'mapping':mapping}
# Symbol order from public zukan_data.json.txt; each expected record matches ROM bytes.
j=json.loads((SRC/'files/application/zukanlist/zkn_data/zukan_data.json').read_text());ms=members('a/0/7/4');spec={k:int(v,0) for k,v in re.findall(r'^#define\s+(SPECIES_\w+)\s+(0x[0-9a-fA-F]+|\d+)\b',(SRC/'include/constants/species.h').read_text(),re.M)}
keys=['height','weight','body_style','scale_f','mon_scale_f','scale_m','mon_scale_m','ypos_f','mon_ypos_f','ypos_m','mon_ypos_m'];labels=['height','weight','body_style','player_scale_f','mon_scale_f','player_scale_m','mon_scale_m','player_ypos_f','mon_ypos_f','player_ypos_m','mon_ypos_m']; expected=[];names=[]
for i,(key,label) in enumerate(zip(keys,labels)):
 vals=[m[key] if isinstance(m[key],int) else m[key]['altered'] for m in j['mon_stats']];fmt='I' if i<2 else 'B' if i==2 else 'H';expected.append(struct.pack('<'+fmt*len(vals),*[v & (0xffffffff if fmt=='I' else 0xff if fmt=='B' else 0xffff) for v in vals]));names.append('mon_stats_'+label)
for group in j['sorting']:
 for sub in group['options']:
  vals=sub['mons'];vals=vals if isinstance(vals,list) else vals['altered'];vals=[v if isinstance(v,int) else spec[v] for v in vals];expected.append(struct.pack('<'+'H'*len(vals),*vals));names.append('sort_order_'+group['type']+'_'+str(sub['id']))
assert len(expected)==len(ms),(len(expected),len(ms))
for name,a,b in zip(names,expected,ms):assert b[:len(a)]==a and all(x==255 for x in b[len(a):]),name
mapping={'NARC_zukan_data_'+name:i for i,name in enumerate(names)}
(out/'zukan_data.naix').write_text('#pragma once\n/* Every member independently matches generated public JSON bytes in real ROM. */\n'+''.join(f'#define {k} {v}\n' for k,v in mapping.items()))
result['zukan_data']={'rom_path':'a/0/7/4','members':len(ms),'source_exact_checks':len(ms),'mapping':mapping}
(HERE/'member-index-audit.json').write_text(json.dumps(result,indent=2)+'\n')
print({k:{x:v[x] for x in ('members','source_exact_checks')} for k,v in result.items()})
