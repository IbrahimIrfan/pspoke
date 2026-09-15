// Shared layouts for the display-list geometry cache (frontend.cpp <-> g3_handler.cpp).
#pragma once
#include "g3_vtx.h"
struct G3SIM_SlotDerived { float sx, sy, sz, zf; u32 color; u8 oob; u8 zValid,finished; unsigned gen; };
struct G3GeomEntryState{u8 color[3],lightFlag;float alpha;u8 lightColor[4][3],diffuse[3],ambient[3],specular[3],emission[3];float lightVector[4][3];fx32 vec[3][3];float texS,texT;u32 sSize,tSize;u8 transform;fx32 texMtx[4][2];};
struct G3SlotState{GXBegin prim;u8 numInPoly,drawPoly;u32 tri,quad,gen;G3SIM_FxVtx_t verts[4];G3SIM_SlotDerived der[4];u8 color[3];float texS,texT;};
extern "C" {
enum{G3DEP_TEX=1,G3DEP_COLOR=2,G3DEP_LIGHT=4,G3DEP_PREV=8,G3DEP_TEXCOORD=16,G3DEP_SLOTS=32};
void G3SIM_CaptureEntryState(G3GeomEntryState*,unsigned deps);int G3SIM_EntryStateMatches(const G3GeomEntryState*,unsigned deps);
void G3SIM_CaptureSlots(G3SlotState*);void G3SIM_RestoreSlots(const G3SlotState*);
int G3SIM_ReplayEligible();void G3SIM_ReplayRawBegin();
void G3ListRecordBegin();int G3ListRecordEnd(unsigned*,const GuVertex**);void G3ListReplay(const GuVertex*,unsigned);void G3ListDrawDirect(const GuVertex*,unsigned);void G3ListDeferFree(void*);void G3ListPendingFreeDrain();
extern unsigned g3NonRawSubmits;
}
