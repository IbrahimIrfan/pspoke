// Safe static batch append; no direct GE-list allocation.
#pragma once
struct GuVertex {float u,v;unsigned color;float x,y,z;};
extern "C" {
extern GuVertex *g3ChunkPtr;extern unsigned g3ChunkLeft;extern unsigned g3RecCount;
void G3SIM_ChunkAlloc(void) __attribute__((noreturn));
}
static inline void G3SIM_AppendVtx(float u,float v,unsigned color,float x,float y,float z){
 if(!g3ChunkLeft)G3SIM_ChunkAlloc();
 GuVertex*p=g3ChunkPtr++;g3ChunkLeft--;g3RecCount++;
 p->u=u;p->v=v;p->color=color;p->x=x;p->y=y;p->z=z;
}
#include <cstring>
#include <stdint.h>
/* Word-wise equality for 4-byte aligned buffers (display lists, VRAM texture/palette ranges).
   newlib's memcmp is a byte loop; this is 4-16x fewer iterations for the same exact answer. */
static inline bool G3FastEqual(const void*a,const void*b,unsigned n){
 if(((((uintptr_t)a)|((uintptr_t)b)|n)&3)==0){const unsigned*x=(const unsigned*)a,*y=(const unsigned*)b;unsigned w=n>>2;
  while(w>=4){if((x[0]^y[0])|(x[1]^y[1])|(x[2]^y[2])|(x[3]^y[3]))return false;x+=4;y+=4;w-=4;}
  while(w){if(*x++!=*y++)return false;w--;}return true;}
 return memcmp(a,b,n)==0;}
