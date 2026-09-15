#!/usr/bin/env python3
"""pathfind.py LOG... -- sx,sz tx,tz [--allow-h]  : BFS over merged [SS-COLL] windows; prints gen_script legs (8 frames/tile)."""
import re,sys
from collections import deque
args=sys.argv[1:]; allowh='--allow-h' in args; args=[a for a in args if a!='--allow-h']
i=args.index('--'); logs=args[:i]; s=tuple(map(int,args[i+1].split(','))); t=tuple(map(int,args[i+2].split(',')))
grid={}
for S in logs:
    ox=None
    for l in open(S,errors='ignore'):
        m=re.search(r'\[SS-COLL\] origin x=(-?\d+) z=(-?\d+)',l)
        if m: ox=int(m.group(1)); continue
        m=re.search(r'\[SS-COLL\] z=(-?\d+) (.*)$',l)
        if m and ox is not None:
            z=int(m.group(1)); row=m.group(2)
            for k,c in enumerate(row.rstrip('\n')):
                if c==' ': continue
                if c=='v' and (k<4 or k>44): continue
                x=ox+k
                if (x,z) not in grid or grid[(x,z)]=='v': grid[(x,z)]=c
def ok(p):
    c=grid.get(p)
    if c is None: return False
    if c=='H': return allowh
    if c in '#O ><^v': return False
    return True
prev={s:None}; q=deque([s])
while q:
    p=q.popleft()
    if p==t: break
    for d in ((1,0),(-1,0),(0,1),(0,-1)):
        n=(p[0]+d[0],p[1]+d[1])
        if n not in prev and ok(n): prev[n]=p; q.append(n)
if t not in prev:
    # nearest reachable to target
    best=min(prev,key=lambda p:abs(p[0]-t[0])+abs(p[1]-t[1])); print('target unreachable; nearest',best,file=sys.stderr); t=best
path=[]; p=t
while p: path.append(p); p=prev[p]
path.reverse()
legs=[]
for a,b in zip(path,path[1:]):
    d=(b[0]-a[0],b[1]-a[1])
    if legs and legs[-1][0]==d: legs[-1][1]+=1
    else: legs.append([d,1])
names={(1,0):'R',(-1,0):'L',(0,1):'D',(0,-1):'U'}
print(' '.join(f"{names[d]}{n*8}" for d,n in legs)); print('end',path[-1],file=sys.stderr)
