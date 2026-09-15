/* Typed native port of original unk_0200FA24.s: SetMasterBrightness,
 * SetMasterBrightnessNeutral, sub_0200FBF4. Native SDK register aliases replace
 * literal DS MMIO; the separate original fade-state halfword remains real data. */
#include <nitro.h>
extern unsigned char ssdata_unk_0200FA24__021D1034[];
void SetMasterBrightness(PMLCDTarget screen,int brightness){
 if(screen==0)GX_SetMasterBrightness(brightness);
 else GXS_SetMasterBrightness(brightness);
}
void SetMasterBrightnessNeutral(PMLCDTarget screen){SetMasterBrightness(screen,0);}
void sub_0200FBF4(PMLCDTarget screen,u16 color){
 if(color==0xffff)color=*(u16*)(ssdata_unk_0200FA24__021D1034+16);
 SetMasterBrightness(screen,color==0x7fff?16:-16);
}

void ResetVisibleHardwareWindows(PMLCDTarget screen){
 if(screen==0)GX_SetVisibleWnd(GX_WNDMASK_NONE);else GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}
