#!/usr/bin/env python3
"""lastmap.py LOG OUT [after_x,z] : keep the [SS-COLL] blocks after the trail first reaches x,z (default: only the last block)."""
import sys,re
lines=open(sys.argv[1],errors='ignore').read().splitlines()
start=0
if len(sys.argv)>3:
    x,z=sys.argv[3].split(',')
    for i,l in enumerate(lines):
        if re.search(rf'fieldinput.*x={x} z={z}\b',l): start=i; break
idx=[i for i,l in enumerate(lines) if '[SS-COLL] origin' in l and i>=start]
if len(sys.argv)<=3: idx=idx[-1:]
out=[]
for i in idx: out+=lines[i:i+70]
open(sys.argv[2],'w').write('\n'.join(out)+'\n'); print(len(idx),'blocks from',lines[idx[0]][-40:] if idx else 'none')
