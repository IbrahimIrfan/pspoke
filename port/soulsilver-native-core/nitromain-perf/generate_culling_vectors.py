from pathlib import Path
import random
b=Path(__file__).resolve().parent;r=random.Random(0x25c98)
def s32(x):return (x+2**31)%2**32-2**31
def mul(a,b):return s32((a*b+2048)>>12)
rows=[]
for k in range(8192):
 attr=r.randrange(65536);bounds=[r.randrange(-32768,32768) for _ in range(4)]
 if k<4096:
  m=[r.randrange(-8192,8193) for _ in range(4)]+[r.randrange(-512,513)*4096 for _ in range(2)]
  v=[r.randrange(-512,513)*4096,r.randrange(-512,513)*4096,256*4096,192*4096]
 else:m=[s32(r.getrandbits(32)) for _ in range(6)];v=[s32(r.getrandbits(32)) for _ in range(4)]
 if k<1024:
  m=[4096,0,0,4096,(k%64-16)*4096,(k//64-8)*4096];v=[0,0,256*4096,192*4096]
 radius=(attr&63)*4
 xlo,xhi=(bounds[2]*4096,bounds[0]*4096) if attr&2048 else (-radius*4096,radius*4096)
 ylo,yhi=(bounds[3]*4096,bounds[1]*4096) if attr&2048 else (-radius*4096,radius*4096)
 xs=sorted(s32(mul(q,m[0])+mul(q,m[2])+s32(m[4]-v[0])) for q in [xlo,xhi])
 ys=sorted(s32(mul(q,m[1])+mul(q,m[3])+s32(m[5]-v[1])) for q in [ylo,yhi])
 expected=int(ys[1]>0 and ys[0]<v[3] and xs[1]>0 and xs[0]<v[2])
 arr=lambda x:'{'+','.join(str(n) for n in x)+'}'
 rows.append('{'+f'{attr},'+arr(bounds)+','+arr(m)+','+arr(v)+f',{expected}'+'}')
(b/'g2d_culling_vectors.h').write_text('static const struct CullingVector {u16 attr;s16 bounds[4];s32 matrix[6],rect[4];u32 expected;} cullingVectors[]={\n'+',\n'.join(rows)+'\n};\n')
