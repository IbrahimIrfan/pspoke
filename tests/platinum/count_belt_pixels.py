#!/usr/bin/env python3
"""count_belt_pixels.py screen.png : exit 0 when the Oreburgh conveyor belts are drawn.
The belts are the only large teal/blue objects in the south of Oreburgh City; count teal pixels
in the main panel (left 363 px) of the PPSSPP screenshot. Old builds culled the belts (0-200
teal pixels); with them drawn there are several thousand."""
import sys, struct, zlib
def png_rgb(path):
    d=open(path,'rb').read(); assert d[:8]==b'\x89PNG\r\n\x1a\n'
    p=8; w=h=0; ct=0; idat=b''
    while p<len(d):
        n,=struct.unpack('>I',d[p:p+4]); t=d[p+4:p+8]; c=d[p+8:p+8+n]; p+=12+n
        if t==b'IHDR': w,h,bd,ct=struct.unpack('>IIBB',c[:10])
        elif t==b'IDAT': idat+=c
    bpp={2:3,6:4}[ct]; raw=zlib.decompress(idat); stride=w*bpp; out=[]; prev=bytearray(stride); q=0
    for y in range(h):
        f=raw[q]; line=bytearray(raw[q+1:q+1+stride]); q+=1+stride
        for i in range(stride):
            a=line[i-bpp] if i>=bpp else 0; b=prev[i]; c=prev[i-bpp] if i>=bpp else 0
            if f==1: line[i]=(line[i]+a)&255
            elif f==2: line[i]=(line[i]+b)&255
            elif f==3: line[i]=(line[i]+(a+b)//2)&255
            elif f==4:
                pa,pb,pc=abs(b-c),abs(a-c),abs(a+b-2*c); pr=a if pa<=pb and pa<=pc else b if pb<=pc else c
                line[i]=(line[i]+pr)&255
        out.append(bytes(line)); prev=line
    return w,h,bpp,out
w,h,bpp,rows=png_rgb(sys.argv[1]); n=0
for y in range(h):
    r=rows[y]
    for x in range(min(w,363)):
        R,G,B=r[x*bpp],r[x*bpp+1],r[x*bpp+2]
        if B>=120 and B>R+60 and G>R+20: n+=1
print('teal pixels:',n); sys.exit(0 if n>=2000 else 1)
