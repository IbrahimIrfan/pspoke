#include <stdio.h>
#include <stdint.h>
extern void __real_BeginNormalPaletteFade(int,int,int,uint16_t,int,int,int);
void __wrap_BeginNormalPaletteFade(int mode,int top,int bottom,uint16_t color,int steps,int period,int heap){printf("[SS-FADE] mode=%d types=%d/%d color=%04x steps=%d period=%d heap=%d\n",mode,top,bottom,color,steps,period,heap);__real_BeginNormalPaletteFade(mode,top,bottom,color,steps,period,heap);}
