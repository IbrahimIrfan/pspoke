#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdio.h>
PSP_MODULE_INFO("Native cadence proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
extern void PSPNativeVBlankWait(void),PSPNativeVBlankReset(void),PSPNativeVBlankFrameComplete(void);
static unsigned measure(int old,unsigned work,unsigned frames){PSPNativeVBlankReset();unsigned start=sceKernelGetSystemTimeLow();for(unsigned i=0;i<frames;i++){if(old){sceDisplayWaitVblankStart();sceDisplayWaitVblankStart();}else{PSPNativeVBlankWait();PSPNativeVBlankWait();}sceKernelDelayThread(work);if(!old)PSPNativeVBlankFrameComplete();}return(sceKernelGetSystemTimeLow()-start)/frames;}
int main(void){sceDisplaySetMode(0,480,272);unsigned old=measure(1,20000,30),fast=measure(0,20000,30),heavy=measure(0,45000,30);int fail=fast<33000||fast>35000||old<fast+10000||heavy<45000||heavy>47500;
/* Light work after a long frame may use one credit, never a burst of catchup. */
unsigned start=sceKernelGetSystemTimeLow();for(int i=0;i<30;i++){PSPNativeVBlankWait();PSPNativeVBlankWait();sceKernelDelayThread(1000);PSPNativeVBlankFrameComplete();}unsigned recover=(sceKernelGetSystemTimeLow()-start)/30;if(recover<31000||recover>34000)fail++;
printf("[CADENCE] failures=%d old_us=%u deadline_us=%u overbudget_us=%u recovery_us=%u\n",fail,old,fast,heavy,recover);sceKernelExitGame();return fail;}
