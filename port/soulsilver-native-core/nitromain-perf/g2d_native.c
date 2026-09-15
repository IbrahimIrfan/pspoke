#include <nitro.h>
#include <nnsys.h>
/* Original unk_02025C44.s native SDK field mappings. */
void GF_InitG2dRenderer(NNSG2dRendererInstance *r,fx32 z){NNS_G2dInitRenderer(r);r->spriteZoffsetStep=z;}
void GF_SetG2dRendererSurface(NNSG2dRenderSurface *s,NNSG2dViewRect *v){s->viewRect=*v;}
void sub_02025C54(NNSG2dRenderSurface *s,NNSG2dViewRect *v,NNSG2dOamRegisterFunction oam,NNSG2dAffineRegisterFunction affine,NNSG2dRndCellCullingFunction cull,NNS_G2D_VRAM_TYPE type,NNSG2dRendererInstance *r){
 NNS_G2dInitRenderSurface(s);s->viewRect=*v;s->pFuncOamRegister=oam;s->pFuncOamAffineRegister=affine;s->pFuncVisibilityCulling=cull;s->type=(NNSG2dSurfaceType)type;if(r)NNS_G2dAddRendererTargetSurface(r,s);
}
