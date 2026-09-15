"""Fail-closed, bounded Thumb source-to-native-C experiment; PSP32 only."""
from pathlib import Path
import re,json,hashlib
HERE=Path(__file__).resolve().parent
SRC=HERE.parent/'soulsilver-research/pokeheartgold-slop/asm'
SELECTION={'unk_02026DE0.s':['sub_02026DE0','sub_02026E18'], 'model_attributes.s':['ModelAttributes_Init']}
INDIRECT_CALLS={}
CALLS={'Heap_Alloc': ('r0=(uint32_t)(uintptr_t)Heap_Alloc((int)r0,r1);',2), 'MIi_CpuClear32': ('MIi_CpuClear32(r0,(void *)(uintptr_t)r1,r2);',3)}
def splitargs(s): return [a.strip() for a in re.split(r',\s*(?![^\[]*\])',s)]
def val(s): return s.removeprefix('#')
def mem(s):
 a=splitargs(s.strip('[]'));return a[0]+(' + '+val(a[1]) if len(a)>1 else '')
def normalize_switches(body):
 lines=[line.split(';')[0].strip() for line in body.splitlines()]
 out=[];i=0
 while i<len(lines):
  m=re.fullmatch(r'add (r[0-7]), (r[0-7]), \2',lines[i])
  if m and i+7<len(lines):
   dst,src=m.groups()
   seq=[f'add {dst}, pc',f'ldrh {dst}, [{dst}, #6]',f'lsl {dst}, {dst}, #0x10',f'asr {dst}, {dst}, #0x10',f'add pc, {dst}']
   if lines[i+1:i+6]==seq and re.fullmatch(r'_[0-9A-Fa-f]{8}:',lines[i+6]):
    table=lines[i+6][:-1];targets=[];j=i+7
    while j<len(lines):
     entry=re.fullmatch(r'\.short (_[0-9A-Fa-f]{8}) - '+table+r' - 2',lines[j])
     if not entry:break
     target=entry.group(1);offset=int(target[1:],16)-int(table[1:],16)-2
     if not -32768<=offset<=32767:raise ValueError('switch target offset outside signed16')
     targets.append(target);j+=1
    if targets:
     out.append('ssswitch '+','.join([dst,src,table]+targets));i=j;continue
  out.append(lines[i]);i+=1
 return '\n'.join(out)
def emit(name,body,relocs=None):
 body=normalize_switches(body)
 relocs=relocs or {}
 literals={};externs=set();clean=[]
 for raw in body.splitlines():
  line=raw.split(';')[0].strip()
  m=re.fullmatch(r'(\w+):\s*\.word\s+(.+)',line)
  if m:
   label,expr=m.groups();expr=expr.strip()
   if re.fullmatch(r'0x[0-9a-fA-F]+|[0-9]+',expr):
    value=int(expr,0)
    if 0x02000000<=value<0x0a000000:raise ValueError('possible fixed DS memory/MMIO literal')
    literals[label]=expr
   elif re.fullmatch(r'[A-Za-z_]\w*(?:\s*[+]\s*(?:0x[0-9a-fA-F]+|[0-9]+))?',expr):
    pieces=re.split(r'\s*[+]\s*',expr);symbol=relocs.get(pieces[0],pieces[0]);externs.add(symbol)
    literals[label]=f'(uint32_t)(uintptr_t){symbol}'+(' + '+pieces[1] if len(pieces)>1 else '')
   else:raise ValueError('unsupported literal expression: '+expr)
  else:clean.append(raw)
 body='\n'.join(clean)
 lines=[l.split(';')[0].strip() for l in body.splitlines()]
 lines=[l for l in lines if l and not l.startswith('.')]
 labels={l[:-1]:i for i,l in enumerate(lines) if l.endswith(':')}
 todo=[(0,0)]; seen={};peak=0
 while todo:
  i,depth=todo.pop()
  if i>=len(lines):raise ValueError('fallthrough without return')
  if i in seen:
   if seen[i]!=depth:raise ValueError('inconsistent stack depth at merge')
   continue
  seen[i]=depth;line=lines[i];op,_,args=line.partition(' ')
  if op in ('push','pop'):depth+=(1 if op=='push' else -1)*len(args.strip('{}').split(','))*4
  elif re.match(r'(add|sub) sp,',line):
   amt=int(args.split('#')[-1],0);depth+=(1 if op=='sub' else -1)*amt
  if depth<0 or depth%4:raise ValueError('unsupported stack layout')
  peak=max(peak,depth)
  if (op=='pop' and 'pc' in args) or line=='bx lr':
   if depth:raise ValueError('unbalanced return stack')
   continue
  if op=='ssswitch':
   fields=args.split(',')
   for target in fields[3:]:
    if target not in labels:raise ValueError('switch target missing')
    todo.append((labels[target],depth))
   continue
  if op in ('b','beq','bne','bhi','blo','blt','bge','bhs','bls','bgt','ble','bpl','bmi'):
   if args not in labels:raise ValueError('external tail branch')
   todo.append((labels[args],depth))
   if op=='b':continue
  todo.append((i+1,depth))
 peak=(peak+3)//4
 out=[*(f'extern unsigned char {symbol}[];' for symbol in sorted(externs)),f'uint32_t native_{name}(uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3) {{', 'uint32_t r0=a0,r1=a1,r2=a2,r3=a3,r4=0,r5=0,r6=0,r7=0,lr=0;', f'uint32_t stack[{max(1,peak)}]; uint32_t sp=(uint32_t)(uintptr_t)(stack+{max(1,peak)});', 'Flags f={0}; uint32_t x,y,res;']
 ops=[]
 for raw in body.splitlines():
  line=raw.split(';')[0].strip()
  if not line: continue
  if line.startswith('.balign '):continue
  if line.startswith('.'):raise ValueError('unsupported directive: '+line)
  if line.endswith(':'):
   if line[:-1]!=name:out.append(line)
   continue
  op,_,args=line.partition(' ');args=args.strip(); a=splitargs(args);ops.append(op)
  if op in ('push','pop'):
   regs=[s.strip() for s in args.strip('{}').split(',')]
   if op=='push':
    out.append(f'sp-={len(regs)*4};')
    out.extend(f'st32(sp+{i*4},{r});' for i,r in enumerate(regs))
   else:
    out.extend(f'{r}=ld32(sp+{i*4});' for i,r in enumerate(regs) if r!='pc')
    out.append(f'sp+={len(regs)*4};')
    if 'pc' in regs:out.append('return r0;')
  elif op in ('ldr','ldrh','ldrb','str','strh','strb','ldrsh','ldrsb'):
   if not a[1].startswith('['):
    if op!='ldr' or a[1] not in literals:raise ValueError('unresolved literal relocation')
    out.append(f'{a[0]}={literals[a[1]]};');continue
   width={'ldr':32,'ldrh':16,'ldrb':8,'str':32,'strh':16,'strb':8,'ldrsh':16,'ldrsb':8}[op]
   out.append(f'{a[0]}='+('(int32_t)(int16_t)' if op=='ldrsh' else '(int32_t)(int8_t)' if op=='ldrsb' else '')+f'ld{width}({mem(a[1])});' if op.startswith('ld') else f'st{width}({mem(a[1])},{a[0]});')
  elif op in ('stmia','ldmia'):
   match=re.fullmatch(r'(r[0-7])!,\s*\{([^}]+)\}',args)
   if not match:raise ValueError('unsupported multiple-transfer syntax')
   base,listed=match.groups();regs=[r.strip() for r in listed.split(',')]
   if base in regs or any(not re.fullmatch(r'r[0-7]',r) for r in regs):raise ValueError('unsupported multiple-transfer register alias/range')
   out.append(f'x={base};')
   for i,reg in enumerate(regs):out.append(f'st32(x+{i*4},{reg});' if op=='stmia' else f'{reg}=ld32(x+{i*4});')
   out.append(f'{base}=x+{len(regs)*4};')
  elif op=='ssswitch':
   dst,src,table,*targets=args.split(',')
   out.append(f'x={src}; res=x+x; af(&f,x,x,res); switch(x) {{')
   for index,target in enumerate(targets):
    offset=int(target[1:],16)-int(table[1:],16)-2
    out.append(f'case {index}: {dst}=(uint32_t)(int32_t)(int16_t){offset}; nz(&f,{dst}); f.c=0; goto {target};')
   out.append('default: __builtin_trap(); }')
  elif op=='mov':
   if not a[1].startswith('#'):raise ValueError('register MOV needs original opcode/flag audit')
   out.append(f'{a[0]}={val(a[1])};')
   # Original Thumb-1 immediate MOV necessarily updates NZ; carry/overflow unchanged.
   out.append(f'nz(&f,{a[0]});')
  elif op in ('add','sub','cmp'):
   lhs=a[0] if len(a)==2 else a[1];rhs=a[-1]
   out.append(f'x={val(lhs)}; y={val(rhs)}; res=x'+('+' if op=='add' else '-')+'y;')
   if op!='cmp':out.append(f'{a[0]}=res;')
   if 'sp' not in a:out.append(('af' if op=='add' else 'sf')+'(&f,x,y,res);')
  elif op=='lsl':
   src=a[0] if len(a)==2 else a[1];count=val(a[-1]);out.append(f'{a[0]}=shift(&f,{src},{count});')
  elif op=='lsr':
   src=a[0] if len(a)==2 else a[1];out.append(f'{a[0]}=shift_right(&f,{src},{val(a[-1])});')
  elif op=='mvn':
   if len(a)!=2 or any(not re.fullmatch(r'r[0-7]',r) for r in a):raise ValueError('unsupported MVN register class')
   out.append(f'{a[0]}=~{a[1]}; nz(&f,{a[0]});') # Thumb-1 MVN updates NZ, preserves CV.
  elif op in ('orr','and','eor','tst','mul'):
   symbol={'orr':'|','and':'&','eor':'^','tst':'&','mul':'*'}[op]
   out.append(f'res={a[0]}{symbol}{a[1]}; nz(&f,res);')
   if op!='tst':out.append(f'{a[0]}=res;')
  elif op in ('beq','bne','bhi','blo','blt','bge','bhs','bls','bgt','ble','bpl','bmi','b'):
   c={'bpl':'!f.n','bmi':'f.n','beq':'f.z','bne':'!f.z','bhi':'f.c&&!f.z','blo':'!f.c','blt':'f.n!=f.v','bge':'f.n==f.v','bhs':'f.c','bls':'!f.c||f.z','bgt':'!f.z&&f.n==f.v','ble':'f.z||f.n!=f.v','b':'1'}[op]
   out.append(f'if ({c}) goto {args};')
  elif op=='nop':pass
  elif op=='bx' and args=='lr':out.append('return r0;')
  elif op=='blx' and (name,args) in INDIRECT_CALLS:
   out.append(INDIRECT_CALLS[(name,args)]);out.append('r1=0xa1a1a1a1; r2=0xa2a2a2a2; r3=0xa3a3a3a3; f=(Flags){0};')
  elif op=='bl' and args in CALLS:
   out.append(CALLS[args][0]);out.append('r1=0xa1a1a1a1; r2=0xa2a2a2a2; r3=0xa3a3a3a3; f=(Flags){0}; /* caller-clobbered */')
  else: raise ValueError(f'{name}: unsupported: {line}')
 out.append('}')
 return '\n'.join(out),ops
prefix='''/* Generated by translate.py. Real native pointers; no ARM bus, scheduler or interpreter. */
#include <stdint.h>
#include <string.h>
typedef char require_32bit_native_pointer[sizeof(void*)==4?1:-1];
typedef struct {unsigned n,z,c,v;} Flags;
static void nz(Flags*f,uint32_t r){f->n=r>>31;f->z=r==0;}
static void af(Flags*f,uint32_t x,uint32_t y,uint32_t r){nz(f,r);f->c=((uint64_t)x+y)>>32;f->v=((~(x^y)&(x^r))>>31);}
static void sf(Flags*f,uint32_t x,uint32_t y,uint32_t r){nz(f,r);f->c=x>=y;f->v=((x^y)&(x^r))>>31;}
static uint32_t shift(Flags*f,uint32_t x,unsigned n){n&=255;if(n){f->c=n<=32?((x>>(32-n))&1):0;x=n<32?x<<n:0;}nz(f,x);return x;}
static uint32_t shift_right(Flags*f,uint32_t x,unsigned n){n&=255;if(n){f->c=n<=32?((x>>(n-1))&1):0;x=n<32?x>>n:0;}nz(f,x);return x;}
#define MEM(N,T) static uint32_t ld##N(uint32_t p){T v;memcpy(&v,(void*)(uintptr_t)p,sizeof(v));return v;} static void st##N(uint32_t p,uint32_t v){T w=v;memcpy((void*)(uintptr_t)p,&w,sizeof(w));}
MEM(8,uint8_t) MEM(16,uint16_t) MEM(32,uint32_t)
extern void *Heap_Alloc(int,uint32_t);
extern void MIi_CpuClear32(uint32_t,void*,uint32_t);
'''
def generate_selected():
 output=[prefix]; manifest=[]
 for fn,names in SELECTION.items():
  text=(SRC/fn).read_text()
  for name in names:
   body=re.search(r'\bthumb_func_start '+name+r'\s*\n(.*?)\bthumb_func_end '+name,text,re.S).group(1)
   code,ops=emit(name,body);output.append(code)
   manifest.append(dict(source=fn,name=name,instructions=len(ops),opcodes=sorted(set(ops)),assembly_sha256=hashlib.sha256(body.encode()).hexdigest()))
 (HERE/'generated.c').write_text('\n\n'.join(output)+'\n')
 (HERE/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print(json.dumps(manifest,indent=2))

if __name__=="__main__":generate_selected()
