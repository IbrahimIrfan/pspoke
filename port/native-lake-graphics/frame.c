extern unsigned long long PSPNativeVBlankTakeSleepTime(void);
#include "../native-render-opt/native_render.h"
#include <stdio.h>
#include <pspkernel.h>
#include <stdlib.h>
#ifndef PSP_NATIVE_PROBE_FRAMES
#define PSP_NATIVE_PROBE_FRAMES 1200
#endif
static unsigned frames;
static unsigned long long start,last,gameUs,audioUs,renderUs;
extern void PSPNativeVBlankFrameComplete(void);
extern void PSPNativeSoundAdvance(unsigned elapsedMicroseconds);
extern void PSPNativeInputGetRenderState(unsigned*,int*,int*,int*,int*);
extern void PSPNativeRenderGetTimings(unsigned*,unsigned*);
extern unsigned RenderStage(unsigned);
extern char PSPNativeOverlayText[128];
extern void PSPNativeG3GetProfile(unsigned*,unsigned*);
void PSPNativeFrameInit(void){start=sceKernelGetSystemTimeWide();int result=PSPNativeRenderInit();if(result){printf("[NATIVE] renderer init failed %d\n",result);abort();}if(PSPNativeRenderBegin())abort();}
void PSPNativeFrameComplete(void){
 unsigned long long now=sceKernelGetSystemTimeWide();if(last)gameUs+=now-last;
 PSPNativeSoundAdvance(33333);unsigned long long a=sceKernelGetSystemTimeWide();audioUs+=a-now;
 unsigned keys;int mode,down,x,y;PSPNativeInputGetRenderState(&keys,&mode,&down,&x,&y);PSPNativeRenderSetInput(keys,mode,down,x,y);
 int result=PSPNativeRenderPresentNoWait();if(result){printf("[NATIVE] unsupported renderer state %d\n",result);abort();}
 renderUs+=sceKernelGetSystemTimeWide()-a;frames++;
 if(frames%30==0){unsigned long long sleepUs=PSPNativeVBlankTakeSleepTime();printf("[HEADROOM] updates=%u sleep_us=%llu uncapped_game_us=%llu\n",frames,sleepUs/30,gameUs>sleepUs?(gameUs-sleepUs)/30:0);unsigned long long us=sceKernelGetSystemTimeWide()-start;{static unsigned long long lastUs;unsigned long long win=us-lastUs;lastUs=us;snprintf(PSPNativeOverlayText,128,"NATIVE %5.1f fps  game %2llu audio %2llu render %2llu ms",win?30000000.0/win:0.0,gameUs/30000,audioUs/30000,renderUs/30000);}{unsigned rb=0,twod=0;PSPNativeRenderGetTimings(&rb,&twod);unsigned g3us=0,g3n=0;PSPNativeG3GetProfile(&g3us,&g3n);printf("[NATIVE-FRAME] updates=%u elapsed_us=%llu render=%u average_fps=%.2f window_game_us=%llu audio_us=%llu render_us=%llu readback_us=%u bind_us=%u draw2d_us=%u g3_us=%u g3_calls=%u\n",frames,us,PSPNativeRenderFrameCount(),us?frames*1000000.0/us:0.0,gameUs/30,audioUs/30,renderUs/30,rb,RenderStage(0),RenderStage(1),g3us/30,g3n/30);}gameUs=audioUs=renderUs=0;}
 if(PSP_NATIVE_PROBE_FRAMES&&frames>=PSP_NATIVE_PROBE_FRAMES){puts("[NATIVE-FRAME] clean bounded probe exit");sceKernelExitGame();return;}
 PSPNativeVBlankFrameComplete();if(PSPNativeRenderBegin())abort();last=sceKernelGetSystemTimeWide();
}
