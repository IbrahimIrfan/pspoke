#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "sound.h"
PSP_MODULE_INFO("SS sound helpers proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
extern const u16 ssdata_unk_02004A44__020F5730[136][2];
void sub_020059E0(int);void sub_02004AB8(u16);void sub_02004A60(u16);u16 GBSounds_GetDSSeqNoByGBSeqNo(u16);u8 GF_GetPlayerNoBySeq(int);void GF_SndHandleSetInitialVolume(s32,s32);int sub_020378CC(void);void sub_02005464(int,int);
static unsigned checks,failures;static unsigned char attrs[64][8] __attribute__((aligned(4)));
#define CHECK(x) do { checks++;if(!(x)){if(failures<10)printf("FAIL %d\n",__LINE__);failures++;}}while(0)
static int events[32],ec,seqArg,handleArg,volumeArg,mode,classified,wireless,nullParam;static NNSSndSeqParam param;static NNSSndHandle handle;
static void event(int x){CHECK(ec<32);if(ec<32)events[ec++]=x;}
void *GF_SdatGetAttrPtr(u32 a){event(100+a);CHECK(a<64);return attrs[a]+2;}
void GF_SetCurrentPlayingBGM(u16 s){event(1);CHECK(s==0);}
const NNSSndSeqParam *NNS_SndArcGetSeqParam(int s){event(2);seqArg=s;return nullParam?NULL:&param;}
NNSSndHandle *GF_GetSoundHandle(int h){event(3);handleArg=h;return &handle;}
void NNS_SndPlayerSetInitialVolume(NNSSndHandle *h,int v){event(4);CHECK(h==&handle);volumeArg=v;}
int sub_0203993C(void){event(5);return mode;}
int sub_02034044(int m){event(6);CHECK(m==mode);return classified;}
int ov00_021E7080(void){event(7);return wireless;}
static void reset(void){ec=0;memset(events,0,sizeof events);}
int main(void){scePowerSetClockFrequency(333,333,166);
 for(unsigned s=0;s<65536;s++){
  u16 expected=s;for(int i=0;i<136;i++)if(ssdata_unk_02004A44__020F5730[i][1]==s){expected=ssdata_unk_02004A44__020F5730[i][0];break;}
  CHECK(GBSounds_GetDSSeqNoByGBSeqNo(s)==expected);
  memset(attrs,0xa5,sizeof attrs);reset();sub_020059E0((int)(s|0xf0120000));CHECK(attrs[0x13][2]==(s&255));attrs[0x13][2]=0xa5;for(unsigned i=0;i<sizeof attrs;i++)CHECK(((u8*)attrs)[i]==0xa5);CHECK(ec==1&&events[0]==119);
  reset();sub_02004A60(s);CHECK(*(u16*)(attrs[10]+2)==(s>1217?expected:s));CHECK(ec==(s>1217?3:2));CHECK(events[0]==110&&events[ec-1]==1);if(s>1217){CHECK(events[1]==158);CHECK(*(u16*)(attrs[58]+2)==s);attrs[58][2]=attrs[58][3]=0xa5;}attrs[10][2]=attrs[10][3]=0xa5;for(unsigned i=0;i<sizeof attrs;i++)CHECK(((u8*)attrs)[i]==0xa5);
  for(nullParam=0;nullParam<2;nullParam++){reset();param.playerNo=s&255;CHECK(GF_GetPlayerNoBySeq(s)==(!s||nullParam?255:(s&255)));CHECK(ec==(s?1:0));if(s)CHECK(seqArg==s&&events[0]==2);}
 }
 int edges[]={INT_MIN,-1000,-1,0,1,126,127,128,255,INT_MAX};
 for(unsigned i=0;i<sizeof edges/sizeof edges[0];i++){reset();GF_SndHandleSetInitialVolume(edges[9-i],edges[i]);CHECK(ec==2&&events[0]==3&&events[1]==4);CHECK(handleArg==edges[9-i]);CHECK(volumeArg==(edges[i]<0?0:edges[i]>127?127:edges[i]));}
 for(int v=0;v<256;v++)for(int h=-1;h<10;h++)for(nullParam=0;nullParam<2;nullParam++)for(classified=0;classified<2;classified++)for(wireless=-1;wireless<3;wireless++){
  reset();param.volume=v;mode=h*37;sub_02005464(v+1,h);CHECK(seqArg==v+1&&events[0]==2);
  int active=!nullParam||h==1||h==8;int update=active&&classified&&wireless==1;int n=1+(active?2:0)+(active&&classified?1:0)+(update?2:0);CHECK(ec==n);
  if(active){CHECK(events[1]==5&&events[2]==6);if(classified)CHECK(events[3]==7);}
  if(update){CHECK(events[n-2]==3&&events[n-1]==4);CHECK(handleArg==h);CHECK(volumeArg==((h==1||h==8?127:v)/5));}
 }
 printf("[SS-SOUND-HELPERS] checks=%u failures=%u\n",checks,failures);sceKernelExitGame();return failures!=0;}
