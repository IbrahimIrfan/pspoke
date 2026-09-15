/* Cooperative 60Hz SDK VBlank deadlines. Presentation still uses PSP vsync. */
#include <pspkernel.h>
#include <stdint.h>
static uint64_t sleepElapsed;
uint64_t PSPNativeVBlankTakeSleepTime(void){uint64_t value=sleepElapsed;sleepElapsed=0;return value;}
static uint64_t epoch,sequence;static int ready;
void PSPNativeVBlankReset(void){ready=0;}
static uint64_t deadline(void){return epoch+(sequence*1000000u)/60u;}
void PSPNativeVBlankWait(void){uint64_t now=sceKernelGetSystemTimeWide();if(!ready){epoch=now;sequence=1;ready=1;}uint64_t target=deadline();while(now<target){uint64_t diff=target-now;unsigned part=diff>1000000?1000000:(unsigned)diff;uint64_t beforeSleep=sceKernelGetSystemTimeWide();sceKernelDelayThread(part);now=sceKernelGetSystemTimeWide();sleepElapsed+=now-beforeSleep;}sequence++;}
/* Bound recovery after sustained over-budget work to one game update. Do this
 * once per completed game frame, not between its two SDK VBlank callbacks. */
void PSPNativeVBlankFrameComplete(void){if(!ready)return;uint64_t now=sceKernelGetSystemTimeWide();if(now>deadline()+33334u){epoch=now-16667u;sequence=0;}}
