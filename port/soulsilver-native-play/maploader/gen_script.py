#!/usr/bin/env python3
"""gen_script.py OUT tokens...  -> replay script starting with Continue (frame 450).
tokens: U/D/L/R<n> hold d-pad n frames; A B T S(start) E(select) X(R) tap; W<n> wait n frames; a<n> tap A every 34 frames n times (B likewise b<n>)."""
import sys
bits={'U':0x10,'D':0x40,'L':0x80,'R':0x20,'A':0x2000,'B':0x4000,'T':0x1000,'S':0x8,'E':0x1,'X':0x200}
out=['0 00000000 128 128','150 00000008 128 128','152 00000000 128 128','300 00000008 128 128','302 00000000 128 128','450 00002000 128 128','452 00000000 128 128']
f=700
def press(b,n): 
    global f; out.append(f'{f} {b:08x} 128 128'); f+=n; out.append(f'{f} 00000000 128 128'); f+=2
for t in sys.argv[2:]:
    k=t[0]; n=int(t[1:]) if len(t)>1 else 0
    if k in 'UDLR': press(bits[k],n)
    elif k=='W': f+=n
    elif k in 'ab': 
        for _ in range(n): press(bits[k.upper()],4); f+=30
    elif k in bits: press(bits[k],4); f+=10
    else: sys.exit('bad token '+t)
out.append(f'{f} 00000000 128 128'); open(sys.argv[1],'w').write('\n'.join(out)+'\n'); print(f+60)
