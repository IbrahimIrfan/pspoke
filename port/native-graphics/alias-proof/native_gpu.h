#pragma once
#include <cstring>
#include <cstdio>
#include "GPU2D_Soft.h"
extern "C" {
extern u8 s_HW_BG_VRAM[0x80000],s_HW_DB_BG_VRAM[0x20000],s_HW_OBJ_VRAM[0x40000],s_HW_DB_OBJ_VRAM[0x20000];
}
namespace GPU {
extern u32 VCount;alignas(8) extern u8 Palette[2048],OAM[2048];
extern u8 *VRAM[4];extern u32 VRAMMap_LCDC;const unsigned VRAMDirtyGranularity=512;
struct Bits {bool v=false;bool&operator[](unsigned){return v;}void SetRange(unsigned,unsigned){}};
struct Tracking {Bits DeriveState(u32*){return{};}};
extern Bits VRAMDirty[4];
extern Tracking VRAMDirty_ABG;extern u32 VRAMMap_ABG[32];inline bool MakeVRAMFlat_ABGCoherent(Bits&){return false;}
extern Tracking VRAMDirty_BBG;extern u32 VRAMMap_BBG[32];inline bool MakeVRAMFlat_BBGCoherent(Bits&){return false;}
extern Tracking VRAMDirty_AOBJ;extern u32 VRAMMap_AOBJ[32];inline bool MakeVRAMFlat_AOBJCoherent(Bits&){return false;}
extern Tracking VRAMDirty_BOBJ;extern u32 VRAMMap_BOBJ[32];inline bool MakeVRAMFlat_BOBJCoherent(Bits&){return false;}
extern Tracking VRAMDirty_ABGExtPal;extern u32 VRAMMap_ABGExtPal[32];inline bool MakeVRAMFlat_ABGExtPalCoherent(Bits&){return false;}
extern Tracking VRAMDirty_BBGExtPal;extern u32 VRAMMap_BBGExtPal[32];inline bool MakeVRAMFlat_BBGExtPalCoherent(Bits&){return false;}
extern Tracking VRAMDirty_AOBJExtPal;extern u32 VRAMMap_AOBJExtPal;inline bool MakeVRAMFlat_AOBJExtPalCoherent(Bits&){return false;}
extern Tracking VRAMDirty_BOBJExtPal;extern u32 VRAMMap_BOBJExtPal;inline bool MakeVRAMFlat_BOBJExtPalCoherent(Bits&){return false;}
extern u8 *VRAMFlat_ABG,*VRAMFlat_BBG,*VRAMFlat_AOBJ,*VRAMFlat_BOBJ;
extern u8 VRAMFlat_ABGExtPal[32768],VRAMFlat_BBGExtPal[32768],VRAMFlat_AOBJExtPal[8192],VRAMFlat_BOBJExtPal[8192];
}
namespace GPU3D {struct R {bool Accelerated=false;};extern R*CurrentRenderer;extern u32 RenderXPos;inline void SetRenderXPos(u32 x){RenderXPos=x;}u32*GetLine(unsigned);}
