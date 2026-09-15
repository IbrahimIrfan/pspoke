#include <nitro.h>
#include <stdio.h>
#include <stdlib.h>
extern void SetMasterBrightness(PMLCDTarget,int);
extern void ResetVisibleHardwareWindows(PMLCDTarget);
extern void SetMasterBrightnessNeutral(PMLCDTarget);
extern void sub_0200FBF4(PMLCDTarget,u16);
extern unsigned char ssdata_unk_0200FA24__021D1034[];
void SSNativeBrightnessProof(void){
 unsigned checks=0,errors=0;u16 oldA=reg_GX_MASTER_BRIGHT,oldB=reg_GXS_DB_MASTER_BRIGHT;
 u16 *fallback=(u16*)(ssdata_unk_0200FA24__021D1034+16),oldColor=*fallback;
 const int screens[]={0,1,2,-1};
 for(unsigned screen=0;screen<4;screen++)for(unsigned choice=0;choice<3;choice++){
  *fallback=choice==0?0x7fff:choice==1?0:0xffff;
  for(unsigned c=0;c<65536;c++){
   reg_GX_MASTER_BRIGHT=0x1234;reg_GXS_DB_MASTER_BRIGHT=0x2345;
   sub_0200FBF4((PMLCDTarget)screens[screen],(u16)c);
   unsigned effective=c==0xffff?*fallback:c;
   u16 expected=effective==0x7fff?0x4010:0x8010;
   checks+=2;if(reg_GX_MASTER_BRIGHT!=(screen==0?expected:0x1234))errors++;
   if(reg_GXS_DB_MASTER_BRIGHT!=(screen==0?0x2345:expected))errors++;
  }
  SetMasterBrightnessNeutral((PMLCDTarget)screens[screen]);checks++;
  if((screen==0?reg_GX_MASTER_BRIGHT:reg_GXS_DB_MASTER_BRIGHT)!=0)errors++;
 }
 u32 oldControlA=reg_GX_DISPCNT,oldControlB=reg_GXS_DB_DISPCNT;
 for(u32 i=0;i<65536;i++)for(unsigned k=0;k<4;k++){
  u32 a=i|(i*0x9e37u<<16),b=~a;reg_GX_DISPCNT=a;reg_GXS_DB_DISPCNT=b;
  ResetVisibleHardwareWindows((PMLCDTarget)screens[k]);checks+=2;
  if(reg_GX_DISPCNT!=(k==0?(a&0xffff1fffu):a))errors++;
  if(reg_GXS_DB_DISPCNT!=(k==0?b:(b&0xffff1fffu)))errors++;
 }
 reg_GX_DISPCNT=oldControlA;reg_GXS_DB_DISPCNT=oldControlB;
 *fallback=oldColor;reg_GX_MASTER_BRIGHT=oldA;reg_GXS_DB_MASTER_BRIGHT=oldB;
 printf("[SS-BRIGHTNESS] checks=%u errors=%u\n",checks,errors);if(errors)abort();
}
