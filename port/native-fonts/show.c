#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <string.h>
#include "font.h"
#include "constants/charcode.h"
static u32 image[256*128] __attribute__((aligned(16)));
static u32 list[4096] __attribute__((aligned(16)));
struct Vertex {short u,v,x,y,z;};
void ShowFonts(void){
 for(unsigned i=0;i<256*128;i++)image[i]=0xff403020;
 for(unsigned row=0;row<3;row++){
  const char*text="PLATINUM";unsigned px=20,py=12+row*38,font=row==2?FONT_UNOWN:row;
  for(unsigned c=0;text[c];c++){
   const TextGlyph*g=Font_TryLoadGlyph(font,CHAR_A+text[c]-'A');
   for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
    unsigned offset=((y/8)*2+x/8)*32+(y&7)*4+(x&7)/2;
    unsigned pal=(g->gfx[offset]>>((x&1)*4))&15;
    if(pal && pal!=15)image[(py+y)*256+px+x]=pal==1?0xfff0f0f0:0xffa06020;
   }px+=g->width+3;
  }
 }
 sceGuInit();sceGuStart(GU_DIRECT,list);sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);sceGuDispBuffer(480,272,(void*)(512*272*4),512);
 sceGuOffset(2048-240,2048-136);sceGuViewport(2048,2048,480,272);sceGuScissor(0,0,480,272);sceGuEnable(GU_SCISSOR_TEST);sceGuDisable(GU_DEPTH_TEST);
 sceGuClearColor(0xff181008);sceGuClear(GU_COLOR_BUFFER_BIT);sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_8888,0,0,0);sceGuTexImage(0,256,128,256,image);sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexWrap(GU_CLAMP,GU_CLAMP);
 struct Vertex*v=sceGuGetMemory(sizeof(*v)*2);v[0]=(struct Vertex){0,0,48,40,0};v[1]=(struct Vertex){256,128,432,232,0};
 sceKernelDcacheWritebackAll();sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,v);sceGuFinish();sceGuSync(0,0);sceDisplayWaitVblankStart();sceGuSwapBuffers();sceGuDisplay(GU_TRUE);sceKernelDelayThread(1000000);
}
