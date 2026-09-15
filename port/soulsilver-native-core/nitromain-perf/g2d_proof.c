#include <nitro.h>
#include <nnsys.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void GF_InitG2dRenderer(NNSG2dRendererInstance*,fx32);
extern void GF_SetG2dRendererSurface(NNSG2dRenderSurface*,NNSG2dViewRect*);
extern void sub_02025C54(NNSG2dRenderSurface*,NNSG2dViewRect*,NNSG2dOamRegisterFunction,NNSG2dAffineRegisterFunction,NNSG2dRndCellCullingFunction,NNS_G2D_VRAM_TYPE,NNSG2dRendererInstance*);
_Static_assert(offsetof(NNSG2dRendererInstance,spriteZoffsetStep)==0x80,"original renderer offset");
_Static_assert(offsetof(NNSG2dRenderSurface,pFuncOamRegister)==0x28,"original OAM offset");
_Static_assert(offsetof(NNSG2dRenderSurface,pFuncVisibilityCulling)==0x34,"original cull offset");
static BOOL Oam(const GXOamAttr *o,u16 i,BOOL d){return TRUE;}
static u16 Affine(const MtxFx22 *m){return 0;}
static BOOL Cull(const NNSG2dCellData*c,const MtxFx32*m,const NNSG2dViewRect*v){return TRUE;}
static u32 seed=0xa8135d79;static u32 Next(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
void SSNativeG2dProof(void){unsigned checks=0,errors=0;
 for(unsigned k=0;k<4096;k++){
  struct{u32 before;NNSG2dRendererInstance r;u32 after;} a,b;
  struct{u32 before;NNSG2dRenderSurface s;u32 after;} s,t;
  memset(&a,0xa5,sizeof(a));b=a;fx32 z=(fx32)Next();
  GF_InitG2dRenderer(&a.r,z);NNS_G2dInitRenderer(&b.r);memcpy((u8*)&b.r+0x80,&z,4);
  checks++;if(memcmp(&a,&b,sizeof(a)))errors++;
  NNSG2dViewRect v={{(fx32)Next(),(fx32)Next()},{(fx32)Next(),(fx32)Next()}};
  memset(&s,0xa5,sizeof(s));t=s;GF_SetG2dRendererSurface(&s.s,&v);memcpy(&t.s,&v,16);checks++;if(memcmp(&s,&t,sizeof(s)))errors++;
  NNS_G2dInitRenderer(&a.r);NNS_G2dInitRenderer(&b.r);memset(&s,0xa5,sizeof(s));t=s;
  NNS_G2D_VRAM_TYPE type=(NNS_G2D_VRAM_TYPE)(k%3);
  sub_02025C54(&s.s,&v,Oam,Affine,Cull,type,k&1?&a.r:NULL);
  NNS_G2dInitRenderSurface(&t.s);memcpy(&t.s,&v,16);t.s.pFuncOamRegister=Oam;t.s.pFuncOamAffineRegister=Affine;t.s.pFuncVisibilityCulling=Cull;t.s.type=(NNSG2dSurfaceType)type;
  checks++;if(memcmp(&s,&t,sizeof(s)))errors++;
  checks++;if((k&1)&&(a.r.pTargetSurfaceList!=&s.s||s.s.pNextSurface))errors++;
 }
 printf("[SS-G2D] checks=%u errors=%u\n",checks,errors);if(errors)abort();
}
