#include "../native-render-opt/native_render.h"
#include <stdio.h>
#include <pspkernel.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#ifndef PSP_NATIVE_PROBE_FRAMES
#define PSP_NATIVE_PROBE_FRAMES 0
#endif
static unsigned frames;
static unsigned long long start,last,gameUs,audioUs,renderUs;
extern void PSPNativeVBlankFrameComplete(void);
extern unsigned long long PSPNativeVBlankIdleTake(void);
extern void PSPNativeSoundAdvance(unsigned elapsedMicroseconds);
extern void PSPNativeInputGetRenderState(unsigned*,int*,int*,int*,int*);
extern void PSPNativeRenderGetTimings(unsigned*,unsigned*);
extern unsigned RenderStage(unsigned);
extern char PSPNativeOverlayText[128];
extern void PSPNativeG3GetProfile(unsigned*,unsigned*);
extern void PSPNativeG3TexProfile(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*);
extern void PSPNativeG3CacheStats(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*,unsigned*,unsigned*);
extern void PSPNativeRenderGeProfile(unsigned*,unsigned*,unsigned*);
extern void PSPNativeG3ListCacheStats(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*,unsigned*,unsigned*);
#ifdef PSP_NATIVE_VERTEX_TRACE
extern void PSPNativeVertexTrace(unsigned);
#endif
extern void PSPNativeMemReport(const char*);
extern void PSPNativeMemPoll(void);
extern void PSPNativeMemLog(const char*,...);
/* [PERF] accumulators: one card-log line per 600 frames (20 s at 30 fps). Everything here
   is a counter that the renderer already maintains; the only per-frame cost is a few adds. */
#ifdef PSP_NATIVE_DEV
/* Frame-length histogram in 60 Hz periods (1,2,3,4,5+) and VBlank waits per game frame (1,2,3,4+): a 30 fps
   game should show every frame in bucket 2 with 2 waits. Printed on the [PERF] line. */
static unsigned frHist[5],waitHist[4],frMaxUs;
#endif
static struct{unsigned long long game,idle,audio,render,g3,ge3d,ge2d,bind,draw2d;unsigned texBinds,texHits,texMisses,texCmp,texEvict,fences,heapHigh,listHits,listRec,listHitVerts;unsigned long long winStart;}perf;
static void PerfLine(void){
 unsigned n=600;unsigned long long now=sceKernelGetSystemTimeWide(),win=now-perf.winStart;perf.winStart=now;
 unsigned tEnt=0,tBytes=0;PSPNativeG3CacheStats(&tEnt,&tBytes,0,0,0,0,0);unsigned lEnt=0,lBytes=0,lHigh=0,lVol=0;PSPNativeG3ListCacheStats(0,0,&lEnt,&lBytes,0,&lHigh,&lVol);
 PSPNativeMemLog("[PERF] frames=%u fps=%.2f game_busy_us=%llu idle_us=%llu audio_us=%llu render_us=%llu g3_us=%llu ge3d_wait_us=%llu ge2d_wait_us=%llu bind_us=%llu draw2d_us=%llu tex_binds=%u tex_hits=%u tex_decodes=%u tex_evict=%u tex_fences=%u tex_cmp_kb=%u tex_cache=%u/%u list_hits=%u list_rec=%u list_hit_verts=%u list_cache=%u/%u list_high=%u list_volatile=%u list_broken=%u heap_used_high=%u",
  frames,win?n*1000000.0/win:0.0,(perf.game-perf.idle)/n,perf.idle/n,perf.audio/n,perf.render/n,perf.g3/n,perf.ge3d/n,perf.ge2d/n,perf.bind/n,perf.draw2d/n,
  perf.texBinds,perf.texHits,perf.texMisses,perf.texEvict,perf.fences,perf.texCmp/1024,tEnt,tBytes,perf.listHits,perf.listRec,perf.listHitVerts,lEnt,lBytes,lHigh,lVol&0xffff,lVol>>16,perf.heapHigh);
#ifdef PSP_NATIVE_DEV
 PSPNativeMemLog("[FRAMES] periods 1/2/3/4/5+=%u/%u/%u/%u/%u waits 1/2/3/4+=%u/%u/%u/%u max_us=%u",frHist[0],frHist[1],frHist[2],frHist[3],frHist[4],waitHist[0],waitHist[1],waitHist[2],waitHist[3],frMaxUs);
 memset(frHist,0,sizeof frHist);memset(waitHist,0,sizeof waitHist);frMaxUs=0;
#endif
 perf.game=perf.idle=perf.audio=perf.render=perf.g3=perf.ge3d=perf.ge2d=perf.bind=perf.draw2d=0;
 perf.texBinds=perf.texHits=perf.texMisses=perf.texCmp=perf.texEvict=perf.fences=0;perf.heapHigh=0;perf.listHits=perf.listRec=perf.listHitVerts=0;
}
void PSPNativeFrameInit(void){start=sceKernelGetSystemTimeWide();perf.winStart=start;int result=PSPNativeRenderInit();if(result){printf("[NATIVE] renderer init failed %d\n",result);abort();}if(PSPNativeRenderBegin())abort();PSPNativeMemReport("frame init");}
void PSPNativeFrameComplete(void){
 unsigned long long now=sceKernelGetSystemTimeWide();if(last)gameUs+=now-last;
#ifdef PSP_NATIVE_DEV
 {static unsigned long long prevEnd;if(prevEnd){unsigned d=(unsigned)(now-prevEnd);unsigned p=(d+8333)/16667;frHist[p<1?0:p>5?4:p-1]++;if(d>frMaxUs)frMaxUs=d;}prevEnd=now;
  extern unsigned PSPNativeVBlankWaitsTake(void);unsigned w=PSPNativeVBlankWaitsTake();waitHist[w<1?0:w>4?3:w-1]++;}
#endif
 /* Real elapsed time, not a fixed 1/30 s: the PSP runs this game at 25-27 fps,
    and sound paced off a nominal frame would starve the sceAudio ring by ~12%.
    Clamped so a long load cannot turn into a burst of pumps. */
#ifdef PSP_NATIVE_AUDIO_FIXEDCLOCK
 PSPNativeSoundAdvance(33333);   /* verification builds only: deterministic dumps */
#else
 {unsigned long long d=last?now-last:33333ULL;if(d<4000ULL)d=4000ULL;else if(d>66666ULL)d=66666ULL;PSPNativeSoundAdvance((unsigned)d);}
#endif
 unsigned long long a=sceKernelGetSystemTimeWide();audioUs+=a-now;
 unsigned keys;int mode,down,x,y;PSPNativeInputGetRenderState(&keys,&mode,&down,&x,&y);PSPNativeRenderSetInput(keys,mode,down,x,y);
 int result=PSPNativeRenderPresentNoWait();if(result){printf("[NATIVE] unsupported renderer state %d\n",result);abort();}
#ifdef PSP_NATIVE_FRAME_DUMP
 /* TEST ONLY: raw dumps of the composed main engine (256x192) and the 3D buffer at chosen frames. */
 {extern unsigned PSPNativeRenderFrameCount(void);unsigned f=PSPNativeRenderFrameCount();
  if((f>=PSP_NATIVE_DUMP_A0&&f<PSP_NATIVE_DUMP_A1&&!(f%PSP_NATIVE_DUMP_STEP))||(f>=PSP_NATIVE_DUMP_B0&&f<PSP_NATIVE_DUMP_B1&&!(f%PSP_NATIVE_DUMP_STEP))){
   extern void*sceGeEdramGetAddr(void);char name[64];snprintf(name,64,"dump%05u.raw",f);FILE*fp=fopen(name,"wb");
   if(fp){const unsigned char*b=(const unsigned char*)sceGeEdramGetAddr();fwrite(b+0x158000,1,256*192*4,fp);fwrite(b+0x188000,1,256*192*4,fp);fwrite(b+0x110000,1,256*192*4,fp);fclose(fp);}}}
#endif
 renderUs+=sceKernelGetSystemTimeWide()-a;frames++;
#ifdef PSP_NATIVE_BURN_US
 {unsigned long long t=sceKernelGetSystemTimeWide();while(sceKernelGetSystemTimeWide()-t<PSP_NATIVE_BURN_US){}} /* TEST ONLY: timing-sensitivity probe */
#endif
#ifdef PSP_NATIVE_VERTEX_TRACE_EVERY
 PSPNativeVertexTrace(frames);
#endif
 if(frames%10==0){PSPNativeMemPoll();struct mallinfo mi=mallinfo();if((unsigned)mi.uordblks>perf.heapHigh)perf.heapHigh=mi.uordblks;}
 if(frames%30==0){unsigned long long us=sceKernelGetSystemTimeWide()-start;
#ifdef PSP_NATIVE_DEV
  {static unsigned long long lastUs;unsigned long long win=us-lastUs;lastUs=us;snprintf(PSPNativeOverlayText,128,"%4.1f fps",win?30000000.0/win:0.0);}
#endif

#ifdef PSP_NATIVE_DEV
  {unsigned rb=0,twod=0;PSPNativeRenderGetTimings(&rb,&twod);unsigned g3us=0,g3n=0;PSPNativeG3GetProfile(&g3us,&g3n);unsigned long long idle=PSPNativeVBlankIdleTake();
   unsigned tb=0,th=0,tm=0,tc=0,te=0;PSPNativeG3TexProfile(&tb,&th,&tm,&tc,&te);unsigned ge2=0,ge3=0,gf=0;PSPNativeRenderGeProfile(&ge2,&ge3,&gf);
   unsigned lh=0,lr=0,le=0,lb=0,lv=0;PSPNativeG3ListCacheStats(&lh,&lr,&le,&lb,&lv,0,0);
   printf("[NATIVE-FRAME] updates=%u elapsed_us=%llu render=%u average_fps=%.2f window_game_us=%llu idle_us=%llu audio_us=%llu render_us=%llu readback_us=%u bind_us=%u draw2d_us=%u g3_us=%u g3_calls=%u ge3d_wait_us=%u ge2d_wait_us=%u tex_binds=%u tex_hits=%u tex_decodes=%u tex_evict=%u tex_fences=%u tex_cmp_kb=%u list_hits=%u list_rec=%u list_hit_verts=%u list_cache=%u/%u\n",frames,us,PSPNativeRenderFrameCount(),us?frames*1000000.0/us:0.0,gameUs/30,idle/30,audioUs/30,renderUs/30,rb,RenderStage(0),RenderStage(1),g3us/30,g3n/30,ge3/30,ge2/30,tb,th,tm,te,gf,tc/1024,lh,lr,lv,le,lb);
#if defined(PSP_NATIVE_VERTEX_TRACE)&&!defined(PSP_NATIVE_VERTEX_TRACE_EVERY)
   PSPNativeVertexTrace(frames);
#endif
   perf.game+=gameUs;perf.idle+=idle;perf.audio+=audioUs;perf.render+=renderUs;perf.g3+=g3us;perf.ge3d+=ge3;perf.ge2d+=ge2;perf.bind+=RenderStage(0)*30ULL;perf.draw2d+=RenderStage(1)*30ULL;
   perf.texBinds+=tb;perf.texHits+=th;perf.texMisses+=tm;perf.texCmp+=tc;perf.texEvict+=te;perf.fences+=gf;perf.listHits+=lh;perf.listRec+=lr;perf.listHitVerts+=lv;}
#endif

  gameUs=audioUs=renderUs=0;}
 /* DEV builds only (PSP_NATIVE_DEV). Hardware speed is unknown: printf is invisible there, so put a line in the card log
    every 20 s of play. Cheap (one open/append/close every 600 frames). */
#ifdef PSP_NATIVE_DEV
 if(frames%600==0){unsigned long long us=sceKernelGetSystemTimeWide()-start;PSPNativeMemLog("[FPS] updates=%u average_fps=%.2f audio_us=%llu",frames,us?frames*1000000.0/us:0.0,perf.audio/600);{extern void PSPNativeSoundOutputLine(char*,unsigned);char line[192];PSPNativeSoundOutputLine(line,sizeof line);PSPNativeMemLog("%s",line);}PerfLine();
#ifdef PSP_NATIVE_GAME_PROF
  {extern void PSPNativeGameProfLine(unsigned);PSPNativeGameProfLine(600);}
#endif
 }
#endif
 if(PSP_NATIVE_PROBE_FRAMES&&frames>=PSP_NATIVE_PROBE_FRAMES){PSPNativeMemReport("probe exit");puts("[NATIVE-FRAME] clean bounded probe exit");sceKernelExitGame();return;}
 PSPNativeVBlankFrameComplete();if(PSPNativeRenderBegin())abort();last=sceKernelGetSystemTimeWide();
}
