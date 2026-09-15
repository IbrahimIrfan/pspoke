import re,sys
grid={}
for S in sys.argv[1:]:
    ox=oz=None
    for l in open(S,errors='ignore'):
        m=re.search(r'\[SS-COLL\] origin x=(-?\d+) z=(-?\d+)',l)
        if m: ox,oz=int(m.group(1)),int(m.group(2)); continue
        m=re.search(r'\[SS-COLL\] z=(-?\d+) (.*)$',l)
        if m and ox is not None:
            z=int(m.group(1)); row=m.group(2)
            for i,c in enumerate(row.rstrip('\n')):
                if c==' ': continue
                x=ox+i
                if c=='v' and (i<4 or i>44): continue
                if c=='@': c='.'
                if (x,z) not in grid or grid[(x,z)]=='v': grid[(x,z)]=c
xs=[x for x,z in grid]; zs=[z for x,z in grid]
x0,x1,z0,z1=min(xs),max(xs),min(zs),max(zs)
print('     '+''.join(str((x//10)%10) for x in range(x0,x1+1))); print('     '+''.join(str(x%10) for x in range(x0,x1+1)))
for z in range(z0,z1+1): print(f'{z:4d} '+''.join(grid.get((x,z),' ') for x in range(x0,x1+1)))
