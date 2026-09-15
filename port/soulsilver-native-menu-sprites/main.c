#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "menu_state.h"
PSP_MODULE_INFO("SS menu progress proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
unsigned char ssdata_overlay_74_thumb_ov74_0223D454[0xea8] __attribute__((aligned(4)));
static unsigned checks,failures;static int drawn,getCalls,setCalls,matrixCalls,renderCalls;static Sprite spr;static int list;
void ov74_022358BC(void);void ov74_022358C8(u32);
#define CHECK(x) do{checks++;if(!(x)){if(failures<8)printf("FAIL %d\n",__LINE__);failures++;}}while(0)
BOOL Sprite_GetDrawFlag(Sprite*s){CHECK(s==&spr);getCalls++;return drawn;}
void Sprite_SetDrawFlag(Sprite*s,BOOL flag){CHECK(s==&spr&&flag==1);setCalls++;drawn=flag;}
VecFx32 *Sprite_GetMatrixPtr(Sprite*s){CHECK(s==&spr);matrixCalls++;return &s->matrix;}
void SpriteList_RenderAndAnimateSprites(SpriteList*l){CHECK(l==(SpriteList*)&list);renderCalls++;}
int main(void){int ys[]={INT_MIN,-1,0,1,0x17ffff,0x180000,0x180001,INT_MAX};
 for(unsigned flags=0;flags<16;flags++)for(unsigned delay=0;delay<4;delay++)for(unsigned y=0;y<8;y++)for(unsigned suppress=0;suppress<3;suppress++){
  memset(MENU,0xa5,0xea8);MENU->sliding=(flags&1)?&spr:NULL;MENU->list=(flags&2)?(SpriteList*)&list:NULL;MENU->delay=delay?delay-1:UINT_MAX;unsigned oldDelay=MENU->delay;drawn=(flags&4)?1:0;int oldDrawn=drawn;spr.matrix=(VecFx32){17,ys[y],23};unsigned char before[0xea8];memcpy(before,MENU,sizeof before);getCalls=setCalls=matrixCalls=renderCalls=0;
  if(suppress==0)ov74_022358BC();else ov74_022358C8(suppress-1);
  int run=(flags&1)&&!oldDelay;CHECK(getCalls==run&&matrixCalls==run);CHECK(setCalls==(run&&!oldDrawn&&suppress<2));CHECK(renderCalls==!!(flags&2));CHECK(MENU->delay==((flags&1)&&oldDelay?oldDelay-1:oldDelay));CHECK((u32)spr.matrix.y==(run&&ys[y]<0x180000?(u32)ys[y]+0x3000:(u32)ys[y]));CHECK(spr.matrix.x==17&&spr.matrix.z==23);MENU->delay=oldDelay;CHECK(!memcmp(before,MENU,sizeof before));
 }
 printf("[SS-MENU-SPRITE] checks=%u failures=%u\n",checks,failures);sceKernelExitGame();return failures!=0;}
