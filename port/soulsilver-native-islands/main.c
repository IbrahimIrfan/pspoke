#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include "field_system.h"
#include "library/spl_emitter.h"
PSP_MODULE_INFO("SS native islands proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
void SS_TestParticle(SPLEmitter *);
void SS_TestGraphics(void);
fx32 SS_TestHeight(FieldSystem *,fx32,fx32,u8 *);
void sub_02054DC8(int,int,VecFx32 *);
REGType16v s_reg_G2_BG0CNT, s_reg_G3X_DISP3DCNT;
REGType32v s_reg_G3_VIEWPORT;
static unsigned failures,checks,fogCalls,clearCalls,planeCalls;
#define CHECK(c) do{checks++;if(!(c)){if(failures<8)printf("failure line %d\n",__LINE__);failures++;}}while(0)
void GF_AssertFail(void){puts("fatal assertion");abort();}
// Test-only dependency spies validate exact calls; never linked in game.
void GfGfx_EngineATogglePlanes(u8 mask,u8 enable){planeCalls++;CHECK(mask==1&&enable==1);}
void G3X_SetFog(BOOL e,GXFogBlend b,GXFogSlope s,int o){fogCalls++;CHECK(e==0&&b==0&&s==0&&o==0);}
void G3X_SetClearColor(GXRgb c,int a,int d,int p,BOOL f){clearCalls++;CHECK(c==0&&a==0&&d==32767&&p==63&&f==0);}
static MapLoadManager *expectManager;static int gotX,gotZ;static BOOL hit;
BOOL ov01_021F654C(MapLoadManager *p,int x,int z,u8 *out){CHECK(p==expectManager&&out==NULL);gotX=x;gotZ=z;return hit;}
static unsigned seed=123;
static unsigned random32(void){seed=seed*1664525u+1013904223u;return seed;}
static int refGrid(int x){unsigned adjusted=(unsigned)x+((unsigned)(x>>15)>>16);return (int)adjusted>>16;}
static unsigned char wfcBuffer[0x760] __attribute__((aligned(32)));
static int wfcStep,wfcHeap,wfcResult,wfcOffset;
int sub_02039FFC(int);
void LoadDwcOverlay(void){CHECK(wfcStep++==0);}
void LoadOVY38(void){CHECK(wfcStep++==1);}
void *Heap_Alloc(enum HeapID heap,u32 size){CHECK(wfcStep++==2&&heap==wfcHeap&&size==0x720);return wfcBuffer+wfcOffset;}
int DWC_Init(void *p){CHECK(wfcStep++==3);CHECK((unsigned)p%32==0);CHECK((unsigned char*)p>=wfcBuffer+wfcOffset&&(unsigned char*)p<wfcBuffer+wfcOffset+32);memset(p,0x3c,0x700);return wfcResult;}
void Heap_Free(void *p){CHECK(wfcStep++==4&&p==wfcBuffer+wfcOffset);}
void UnloadDwcOverlay(void){CHECK(wfcStep++==5);}
void UnloadOVY38(void){CHECK(wfcStep++==6);}
int main(void){
 scePowerSetClockFrequency(333,333,166);
 CHECK(offsetof(SPLEmitter,p_res)==0x20);CHECK(offsetof(SPLEmitter,emtr_pos)==0x28);
 CHECK(offsetof(SPLResource,p_base)==0);CHECK(offsetof(SPLResBase,pos)==4);
 CHECK(offsetof(FieldSystem,mapLoadManager)==0x2c);
 for(unsigned r=0;r<65536;r++){
  s_reg_G2_BG0CNT=r;s_reg_G3X_DISP3DCNT=r;s_reg_G3_VIEWPORT=0;
  SS_TestGraphics();unsigned ref=r;ref&=0xcffd;ref=(ref&0xcfff)|16;ref&=0xcffb;ref=(ref&0xcfff)|8;ref&=0xcfdf;
  CHECK(s_reg_G2_BG0CNT==((r&~3)|1));CHECK(s_reg_G3X_DISP3DCNT==ref);CHECK(s_reg_G3_VIEWPORT==0xbfff0000);
 }
 CHECK(fogCalls==65536&&clearCalls==65536&&planeCalls==65536);
 int edge[]={INT_MIN,INT_MIN+1,-65537,-65536,-65535,-1,0,1,65535,65536,65537,INT_MAX};
 FieldSystem f;memset(&f,0,sizeof f);expectManager=(MapLoadManager*)&f;f.mapLoadManager=expectManager;
 for(unsigned i=0;i<20000;i++){
  int x=i<12?edge[i]:(int)random32(),z=i<12?edge[11-i]:(int)random32();hit=(i%3)?7:0;u8 out=9;
  CHECK(SS_TestHeight(&f,x,z,&out)==0);CHECK(out==(hit!=0));CHECK(gotX==refGrid(x)&&gotZ==refGrid(z));
  CHECK(SS_TestHeight(&f,x,z,NULL)==0);
  int width=1+random32()%65535;VecFx32 v={0,0x12345678,0};sub_02054DC8(x,width,&v);
  long long q=(long long)x/width,r=(long long)x%width;
  CHECK((unsigned)v.x==(unsigned)(0x100000+(r&65535)*2097152));CHECK((unsigned)v.z==(unsigned)(0x100000+(q&65535)*2097152));CHECK(v.y==0x12345678);
  SPLResBase base;SPLResource resource;SPLEmitter emitter,before;
  memset(&base,0,sizeof base);memset(&resource,0,sizeof resource);memset(&emitter,0xa5,sizeof emitter);
  base.pos.x=x;base.pos.y=z;base.pos.z=(int)random32();resource.p_base=&base;emitter.p_res=&resource;before=emitter;
  SS_TestParticle(&emitter);CHECK(emitter.emtr_pos.x==x);CHECK((unsigned)emitter.emtr_pos.y==(unsigned)z+0x560);CHECK(emitter.emtr_pos.z==base.pos.z);
  before.emtr_pos=emitter.emtr_pos;CHECK(!memcmp(&before,&emitter,sizeof emitter));
 }
 for(int off=0;off<32;off++)for(int status=-4;status<8;status++){
  memset(wfcBuffer,0xa5,sizeof wfcBuffer);wfcStep=0;wfcOffset=off;wfcHeap=off+status;wfcResult=status;
  CHECK(sub_02039FFC(wfcHeap)==status);CHECK(wfcStep==7);
  for(unsigned i=0;i<(unsigned)off;i++)CHECK(wfcBuffer[i]==0xa5);
  for(unsigned i=off+0x720;i<sizeof wfcBuffer;i++)CHECK(wfcBuffer[i]==0xa5);
 }
 printf("[SS-ISLANDS] checks=%u failures=%u\n",checks,failures);sceKernelExitGame();return failures!=0;
}
