/* Cooperative 60Hz SDK VBlank deadlines. Presentation still uses PSP vsync. */
#include <pspkernel.h>
#include <stdint.h>
static uint64_t epoch,sequence,idleUs;static int ready;
/* Time spent sleeping for a VBlank deadline; the frame profiler subtracts it from the game window. */
unsigned long long PSPNativeVBlankIdleTake(void){uint64_t v=idleUs;idleUs=0;return v;}
static unsigned waitCount; unsigned PSPNativeVBlankWaitsTake(void){unsigned v=waitCount;waitCount=0;return v;} /* DEV histogram: waits per game frame */
void PSPNativeVBlankReset(void){ready=0;}
/* Fast-forward (SELECT+SQUARE, frame.c): `speed` VBlanks per 1/60 s. A change re-anchors the clock at the next wait. */
static unsigned speed=1;
void PSPNativeVBlankSetSpeed(unsigned s){if(s<1)s=1;if(s!=speed){speed=s;ready=0;}}
static uint64_t deadline(void){return epoch+(sequence*1000000u)/(60u*speed);}
void PSPNativeVBlankWait(void){waitCount++;uint64_t now=sceKernelGetSystemTimeWide();if(!ready){epoch=now;sequence=1;ready=1;}uint64_t target=deadline();if(now<target)idleUs+=target-now;while(now<target){uint64_t diff=target-now;unsigned part=diff>1000000?1000000:(unsigned)diff;sceKernelDelayThread(part);now=sceKernelGetSystemTimeWide();}sequence++;}
/* Bound recovery after sustained over-budget work to one game update. Do this
 * once per completed game frame, not between its two SDK VBlank callbacks. */
void PSPNativeVBlankFrameComplete(void){if(!ready)return;uint64_t now=sceKernelGetSystemTimeWide();if(now>deadline()+33334u){epoch=now-16667u;sequence=0;}}
