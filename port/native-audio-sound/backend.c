#include <nitro.h>
#include <nitro/snd/common/work.h>
#include <simulator/sim_audio.h>
#include <stdio.h>
#include <pspkernel.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define SND_MSG_REQUEST_COMMAND_PROC 0
#define SND_COMMAND_NUM 256
#ifndef PSP_AUDIO_MONO
#define PSP_AUDIO_MONO 1
#endif
#ifndef PSP_AUDIO_GAIN_SHIFT
#if PSP_AUDIO_MONO
#define PSP_AUDIO_GAIN_SHIFT 1   /* mono puts the whole sum on both speakers */
#else
#define PSP_AUDIO_GAIN_SHIFT 2
#endif
#endif
/* Output gain. The reference DS mixer this decoder came from scales each
   channel by volume/128 then pan/1024, which peaks 16 dB below full scale on
   real Platinum content (measured: -16.4 dBFS peak, -35.2 dBFS RMS over the
   intro, title and overworld). Two bits of gain put the peak near -4 dBFS
   with the hard clamp below as the only limiter. */
/* Masking: a channel that cannot reach peak>>SHIFT output units is skipped.
   4 = 24 dB below the running peak of the mix. */
#ifndef PSP_AUDIO_QUIET_SHIFT
#define PSP_AUDIO_QUIET_SHIFT 4
#endif
#ifndef PSP_AUDIO_QUIET_MIN
#define PSP_AUDIO_QUIET_MIN 32
#endif
#ifndef PSP_AUDIO_QUIET_MAX
#define PSP_AUDIO_QUIET_MAX 256
#endif
#define UNPACK_COMMAND(v,shift,bits) (((v)>>(shift))&((1ull<<(bits))-1))
static PXIFifoCallback callback;
static BOOL initialized;
static unsigned lists,commands,pumps,activeSeen,tickPeak;static u32 sampleHash=2166136261u;static u64 samples,energy,pumpMicros,clockRemainder,cyclePending;
extern void SNDi_SetPlayerParam7(int,u32,u32,int);
extern void SNDi_SetTrackParam7(int,u32,u32,u32,int);
extern void SNDi_SetSurroundDecay7(int);
extern void SND_LockChannel7(u32,u32);extern void SND_UnlockChannel7(u32,u32);extern void SND_StopUnlockedChannel7(u32,u32);
extern void SND_SetMasterPan7(int);

/* ---- speaker output ---------------------------------------------------------
 * PSPNativeSoundSilent stays 1 (the cheap arithmetic channel advance, no
 * decode, no mix) unless sceAudio actually gave us a channel. A failure to
 * reserve one is logged and the game runs exactly as it did before.
 */
#define PSP_AUDIO_PUMP_CYCLES (SND_PROC_INTERVAL * 64u)     /* 174592 ARM7 cycles */
#ifndef PSP_AUDIO_OUT_FREQ
#define PSP_AUDIO_OUT_FREQ    16000u
#endif
#define PSP_AUDIO_MAX_BLOCK   192                            /* > 174592/1047 */
extern void PSPNativeAudioMixBlock(s32 *outL, s32 *outR, unsigned n);
extern unsigned PSPNativeAudioMixActiveMask(void);
extern int PSPNativeAudioOutInit(void);
extern int PSPNativeAudioOutReady(void);
extern void PSPNativeAudioOutWrite(const short *interleaved, unsigned frames);
extern unsigned PSPNativeAudioOutFill(void);
extern unsigned PSPNativeAudioOutTargetFill(void);
extern unsigned PSPNativeAudioOutCapacity(void);
extern void PSPNativeAudioOutStats(unsigned *, unsigned *, unsigned *, unsigned *);
extern void PSPNativeMemLog(const char *fmt, ...);
#ifdef PSP_NATIVE_AUDIO_WAV
extern void PSPNativeAudioDump(const short *interleaved, unsigned frames);
#endif
static int outputReady;
static u64 blockAcc;                 /* fractional output samples carried between pumps */
static s32 mixL[PSP_AUDIO_MAX_BLOCK], mixR[PSP_AUDIO_MAX_BLOCK];
static short mixOut[PSP_AUDIO_MAX_BLOCK * 2];
static unsigned mixPeak, mixClips, peakEnv;

static void Unsupported(unsigned id){printf("[AUDIO] unsupported command %u\n",id);abort();}
static void SetLocal(int p,int v,s16 n){if(p<0||p>=SND_PLAYER_NUM||v<0||v>=SND_PLAYER_VARIABLE_NUM)abort();SNDi_SharedWork->player[p].variable[v]=n;}
static void SetGlobal(int v,s16 n){if(v<0||v>=SND_GLOBAL_VARIABLE_NUM)abort();SNDi_SharedWork->globalVariable[v]=n;}
static void SetVolume(int v){if(v<0||v>127)abort();s_reg_SND_SOUNDCNT=(s_reg_SND_SOUNDCNT&~127)|v;}
static void SetOutput(SNDOutput l,SNDOutput r,SNDChannelOut a,SNDChannelOut b){s_reg_SND_SOUNDCNT=(s_reg_SND_SOUNDCNT&~0x3f00)|(l<<8)|(r<<10)|(a<<12)|(b<<13);}
static void SetChannelTimer(u32 mask,int t){for(int i=0;i<16;i++)if(mask&(1u<<i))SND_SetChannelTimer7(i,t);}
static void SetChannelVolume(u32 mask,int v,SNDChannelDataShift s){for(int i=0;i<16;i++)if(mask&(1u<<i))SND_SetChannelVolume7(i,v,s);}
static void SetChannelPan(u32 mask,int p){for(int i=0;i<16;i++)if(mask&(1u<<i))SND_SetChannelPan7(i,p);}
static void StartTimer(u32 ch,u32 capture,u32 alarm,u32 flags){if(capture||alarm)Unsupported(SND_COMMAND_START_TIMER);for(int i=0;i<16;i++)if(ch&(1u<<i))SND_StartChannel7(i);}
static void StopTimer(u32 ch,u32 capture,u32 alarm,u32 flags){if(capture||alarm)Unsupported(SND_COMMAND_STOP_TIMER);for(int i=0;i<16;i++)if(ch&(1u<<i))SND_StopChannel7(i,flags);}
static void ReadDriverInfo(SNDDriverInfo*out){memcpy(&out->work,&SNDi_Work,sizeof(SNDi_Work));memcpy(out->chCtrl,s_SIM_sndcnt,sizeof(out->chCtrl));out->workAddress=&SNDi_Work;out->lockedChannels=SND_GetLockedChannel(0);}
void PSPNativeSoundInit(void){if(initialized)return;SND_ExChannelInit();SND_SeqInit();s_reg_SND_SOUNDCNT=0x807f;initialized=TRUE;
 extern int PSPNativeSoundSilent;
#ifdef PSP_NATIVE_SAS
 { extern int PSPNativeSasInit(void); outputReady = (PSPNativeSasInit() == 0); }
 PSPNativeSoundSilent = 1;   /* the CPU mixer never runs: channels advance silently, sceSas plays them */
 PSPNativeMemLog("[AUDIO] init output=%s (sceSasCore) sound=%s", outputReady ? "ready" : "unavailable", outputReady ? "on" : "off");
 return;
#endif
 outputReady = (PSPNativeAudioOutInit() == 0);
#if defined(PSP_NATIVE_AUDIO_OFF)
 PSPNativeSoundSilent = 1;   /* measurement build: the pre-audio silent baseline */
#elif defined(PSP_NATIVE_AUDIO_WAV) || defined(PSP_NATIVE_AUDIO_NOOUT)
 PSPNativeSoundSilent = 0;   /* measurement build: always mix, even with no channel */
#elif defined(PSP_NATIVE_AUDIO_DEFAULT_ON)
 PSPNativeSoundSilent = outputReady ? 0 : 1;
#else
 /* Default OFF: on real hardware the first audio build dropped the game from ~26 to
    ~21 fps (cause not yet isolated; PPSSPP predicted +0.4 ms). Sound is one hotkey away
    (SELECT+SQUARE, see input.c) so the player chooses. */
 PSPNativeSoundSilent = 1;
#endif
 PSPNativeMemLog("[AUDIO] init output=%s sound=%s", outputReady ? "ready" : "unavailable", PSPNativeSoundSilent ? "off" : "on");
}
/* Runtime hotkey. Returns the new state: 1 = sound on. */
int PSPNativeSoundToggle(void){
 extern int PSPNativeSoundSilent;
 if(!outputReady){PSPNativeMemLog("[AUDIO] toggle ignored: output unavailable");return 0;}
#ifdef PSP_NATIVE_SAS
 { extern int PSPNativeSasToggleMute(void); int on=PSPNativeSasToggleMute(); PSPNativeMemLog("[AUDIO] sceSas sound %s", on ? "on" : "off"); return on; }
#endif
 PSPNativeSoundSilent=!PSPNativeSoundSilent;
 PSPNativeMemLog("[AUDIO] sound %s", PSPNativeSoundSilent ? "off" : "on");
 return !PSPNativeSoundSilent;
}
void PXI_InitFifo(void){PSPNativeSoundInit();}
void PXI_SetFifoRecvCallback(int tag,PXIFifoCallback fn){if(tag!=PXI_FIFO_TAG_SOUND){printf("[AUDIO] unsupported FIFO callback %d\n",tag);abort();}callback=fn;PSPNativeSoundInit();}
BOOL PXI_IsCallbackReady(int tag,PXIProc proc){return tag==PXI_FIFO_TAG_SOUND&&initialized;}
int PXI_SendWordByFifo(int tag,u64 data,BOOL error){
 if(tag!=PXI_FIFO_TAG_SOUND||error){printf("[AUDIO] unsupported FIFO send %d\n",tag);return -1;}
 if(data==SND_MSG_REQUEST_COMMAND_PROC)return 0;
 if(data<4096||data>UINT32_MAX||(data&3))abort();
 const SNDCommand*p=(const SNDCommand*)(uintptr_t)data;unsigned limit=0;
 while(p){if(++limit>SND_COMMAND_NUM)abort();SNDCommand command=*p;commands++;
 #include "consumer_switch.inc"
 p=command.next;
 }
 if(!SNDi_SharedWork)abort();SNDi_SharedWork->finishCommandTag++;lists++;return 0;
}
void PSPNativeSoundPump(void){
 u64 started=sceKernelGetSystemTimeWide();
#ifdef PSP_NATIVE_AUDIO_PROFILE
 static u64 tUpd,tSeq,tEx,tLoop,tStat;u64 t0=started,t1;
 SND_UpdateExChannel();t1=sceKernelGetSystemTimeWide();tUpd+=t1-t0;t0=t1;
 SND_SeqMain(TRUE);t1=sceKernelGetSystemTimeWide();tSeq+=t1-t0;t0=t1;
 SND_ExChannelMain(TRUE);t1=sceKernelGetSystemTimeWide();tEx+=t1-t0;t0=t1;pumps++;
#else
 SND_UpdateExChannel();SND_SeqMain(TRUE);SND_ExChannelMain(TRUE);pumps++;
#endif
 // One Nitro sound interval is 2728 DS OS ticks: 174592 ARM7 cycles.
 // Inactive channels return zero before modifying any sample-engine state.
 // Only the sequencer above can start a channel during this synchronous pump.
 unsigned active=0;
 for(unsigned ch=0;ch<16;ch++)if(s_SIM_sndcnt[ch]&(1u<<31))active|=1u<<ch;
 extern int PSPNativeSoundSilent;extern void SIM_Audio_AdvanceChannelSilent(u32,u32,int);extern void SIM_Audio_VerifySilentAdvance(u32,u32,int);
 if(PSPNativeSoundSilent){
  // Muted: no sample is decoded or mixed. Each busy channel's timer, position and
  // enable bit advance arithmetically to the values the chunked decoder would leave.
  for(unsigned pending=active;pending;){unsigned ch=__builtin_ctz(pending);pending&=pending-1;
#ifdef PSP_NATIVE_AUDIO_VERIFY
   SIM_Audio_VerifySilentAdvance(174592,512,ch);
#else
#ifdef PSP_NATIVE_SAS
   /* DS channel timers count at 16756991/s (sample rate = 16756991/timer) while a sound pump is
      174592 units of the 33514000/s system clock (5.21 ms), so a pump is 87296 channel-timer units.
      Advancing 174592 made channels run and end twice as fast as the sceSas voices that play them. */
   SIM_Audio_AdvanceChannelSilent(87296,256,ch);
#else
   SIM_Audio_AdvanceChannelSilent(174592,512,ch);
#endif
#endif
  }
  samples+=(174592/512)*16;
 }else{
  /* Real output. One block per pump: PSP_AUDIO_PUMP_CYCLES of DS time mixed at
   * one output sample per PSP_AUDIO_STEP_CYCLES (1047) ARM7 cycles, which is
   * exactly PSP_AUDIO_OUT_FREQ samples per second of DS time. The fractional
   * sample per pump is carried in blockAcc so the long-run rate is exact. */
  blockAcc += (u64)PSP_AUDIO_OUT_FREQ * PSP_AUDIO_PUMP_CYCLES;
  unsigned n = (unsigned)(blockAcc / (u64)OS_SYSTEM_CLOCK);
  blockAcc -= (u64)n * (u64)OS_SYSTEM_CLOCK;
  if(n > PSP_AUDIO_MAX_BLOCK) n = PSP_AUDIO_MAX_BLOCK;
  if(n){
   unsigned blockPeak=0;
   memset(mixL,0,n*sizeof(s32));
#if PSP_AUDIO_MONO
   /* Cheap mode: the mixer never writes mixR. One clamp, one store pair. */
   PSPNativeAudioMixBlock(mixL,mixR,n);
   for(unsigned i=0;i<n;i++){
    s32 l=mixL[i]<<PSP_AUDIO_GAIN_SHIFT;
    if(l>32767){l=32767;mixClips++;}else if(l<-32768){l=-32768;mixClips++;}
    mixOut[i*2]=mixOut[i*2+1]=(short)l;
    {unsigned a=(unsigned)(l<0?-l:l);if(a>blockPeak)blockPeak=a;}
   }
#else
   memset(mixR,0,n*sizeof(s32));
   PSPNativeAudioMixBlock(mixL,mixR,n);
   for(unsigned i=0;i<n;i++){
    s32 l=mixL[i]<<PSP_AUDIO_GAIN_SHIFT, r=mixR[i]<<PSP_AUDIO_GAIN_SHIFT;
    if(l>32767){l=32767;mixClips++;}else if(l<-32768){l=-32768;mixClips++;}
    if(r>32767){r=32767;mixClips++;}else if(r<-32768){r=-32768;mixClips++;}
    mixOut[i*2]=(short)l;mixOut[i*2+1]=(short)r;
    {unsigned a=(unsigned)(l<0?-l:l);if(a>blockPeak)blockPeak=a;}
   }
#endif
   if(blockPeak>mixPeak)mixPeak=blockPeak;energy+=blockPeak;
   /* Envelope of the mix peak: instant attack, ~1/32 per block decay (about
      170 ms to fall by half), used as the masking reference for the next block. */
   peakEnv=blockPeak>peakEnv?blockPeak:peakEnv-(peakEnv>>5);
   {extern int PSPNativeAudioQuietLevel;int q=(int)(peakEnv>>PSP_AUDIO_QUIET_SHIFT);
    if(q<PSP_AUDIO_QUIET_MIN)q=PSP_AUDIO_QUIET_MIN;else if(q>PSP_AUDIO_QUIET_MAX)q=PSP_AUDIO_QUIET_MAX;
    PSPNativeAudioQuietLevel=q;}
   if(outputReady)PSPNativeAudioOutWrite(mixOut,n);
#ifdef PSP_NATIVE_AUDIO_WAV
   PSPNativeAudioDump(mixOut,n);
#endif
  }
  samples+=n;
 }
#ifdef PSP_NATIVE_AUDIO_PROFILE
 t1=sceKernelGetSystemTimeWide();tLoop+=t1-t0;t0=t1;
#endif
#ifdef PSP_NATIVE_SAS
 {extern void PSPNativeSasSnapshot(u32);PSPNativeSasSnapshot(174592);}
#endif
 if(SNDi_SharedWork){unsigned mask=0;for(int ch=0;ch<16;ch++)if(SND_IsChannelActive7(ch))mask|=1u<<ch;activeSeen|=mask;SNDi_SharedWork->channelStatus=mask;SNDi_SharedWork->captureStatus=0;for(unsigned i=0;i<SND_PLAYER_NUM;i++)if(SNDi_SharedWork->player[i].tickCounter>tickPeak)tickPeak=SNDi_SharedWork->player[i].tickCounter;}
#ifdef PSP_NATIVE_AUDIO_PROFILE
 t1=sceKernelGetSystemTimeWide();tStat+=t1-t0;
 if(pumps%192==0){printf("[AUDIO-PROFILE] pumps=%u upd=%llu seq=%llu ex=%llu loop=%llu stat=%llu active=%04x\n",pumps,tUpd,tSeq,tEx,tLoop,tStat,active);
#ifdef PSP_NATIVE_AUDIO_VERIFY
 {extern unsigned PSPNativeSoundSilentVerifyChecks(void),PSPNativeSoundSilentVerifyMismatches(void),PSPNativeSoundSilentVerifyStops(void);printf("[AUDIO-VERIFY] checks=%u mismatches=%u stops=%u\n",PSPNativeSoundSilentVerifyChecks(),PSPNativeSoundSilentVerifyMismatches(),PSPNativeSoundSilentVerifyStops());}
#endif
 tUpd=tSeq=tEx=tLoop=tStat=0;}
#endif
 pumpMicros+=sceKernelGetSystemTimeWide()-started;
}
void PSPNativeSoundReport(void){printf("[AUDIO] lists=%u commands=%u pumps=%u channelSteps=%llu energy=%llu players=%08lx\n",lists,commands,pumps,samples,energy,SNDi_SharedWork?(unsigned long)SNDi_SharedWork->playerStatus:0);}

/* Called once per rendered frame with the microseconds of real time that the
 * frame took. Sound is paced by that clock, not by a fixed 1/30 s, so a PSP
 * running the game at 26 fps still produces samples at real-time speed and the
 * sceAudio ring neither starves nor overflows. Two guards on top of that:
 * a hard cap on pumps per call (a long load must not turn into a CPU spike),
 * and a soft correction from the measured ring fill, which is the only signal
 * that tells us the real drain rate of the hardware audio channel. */
void PSPNativeSoundAdvance(u32 microseconds){
 clockRemainder+=(u64)microseconds*OS_SYSTEM_CLOCK;cyclePending+=clockRemainder/1000000;clockRemainder%=1000000;
#ifndef PSP_NATIVE_SAS
 if(outputReady){
  /* Trim by at most one pump per frame (about +-16% of the sample rate) toward
   * the target ring fill. Bounded on purpose: the real-time clock above is the
   * authority, this only absorbs the slow drift between the DS clock constant
   * and the PSP audio clock, and it cannot run away if a host drains the
   * channel at the wrong rate. */
  unsigned fill=PSPNativeAudioOutFill(),target=PSPNativeAudioOutTargetFill();
  if(fill>target*2u||fill>(PSPNativeAudioOutCapacity()*3u)/4u)
   cyclePending=cyclePending>PSP_AUDIO_PUMP_CYCLES?cyclePending-PSP_AUDIO_PUMP_CYCLES:0;
  else if(fill<target)cyclePending+=PSP_AUDIO_PUMP_CYCLES;
 }
#endif
 unsigned budget=10;
 while(cyclePending>=PSP_AUDIO_PUMP_CYCLES&&budget){PSPNativeSoundPump();cyclePending-=PSP_AUDIO_PUMP_CYCLES;budget--;}
 if(cyclePending>=PSP_AUDIO_PUMP_CYCLES)cyclePending=0;                          /* hit the cap: do not try to catch up */
}
void PSPNativeSoundOutputLine(char*buf,unsigned len){
#ifdef PSP_NATIVE_SAS
 {extern void PSPNativeSasStatsLine(char*,unsigned);PSPNativeSasStatsLine(buf,len);return;}
#endif
unsigned fill=0,under=0,drop=0,written=0;PSPNativeAudioOutStats(&fill,&under,&drop,&written);
#ifdef PSP_NATIVE_AUDIO_STATS
 {extern void PSPNativeAudioMixStats(unsigned*,unsigned*,unsigned*,unsigned*);unsigned d=0,sm=0,cb=0,mx=0;PSPNativeAudioMixStats(&d,&sm,&cb,&mx);
  extern void PSPNativeAudioMixStats2(unsigned*,unsigned*,unsigned*);unsigned dt[5],bt[5],q[3];PSPNativeAudioMixStats2(dt,bt,q);
  snprintf(buf,len,"[AUDIO] peak=%u clips=%u active=%04x dec=%u smp=%u cb=%u livemax=%u dtype=%u/%u/%u/%u/%u btype=%u/%u/%u/%u/%u quietblk=%u quietdec=%u centre=%u",mixPeak,mixClips,PSPNativeAudioMixActiveMask(),d,sm,cb,mx,dt[0],dt[1],dt[2],dt[3],dt[4],bt[0],bt[1],bt[2],bt[3],bt[4],q[0],q[1],q[2]);return;}
#endif
 snprintf(buf,len,"[AUDIO] out=%d fill=%u written=%u underruns=%u dropped=%u peak=%u clips=%u active=%04x",outputReady,fill,written,under,drop,mixPeak,mixClips,PSPNativeAudioMixActiveMask());}
BOOL PSPNativeSoundProofValid(void){printf("[AUDIO-STATE] activeSeen=%04x tickPeak=%u pumpMicros=%llu averageUs=%llu\n",activeSeen,tickPeak,pumpMicros,pumps?pumpMicros/pumps:0);extern u32 PSPNativeAudioStateHash(void);printf("[AUDIO-HASH] samples=%08lx state=%08lx\n",(unsigned long)sampleHash,(unsigned long)PSPNativeAudioStateHash());return energy>0&&activeSeen&&tickPeak&&SNDi_SharedWork&&SNDi_SharedWork->finishCommandTag==lists;}
