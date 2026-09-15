#include <nitro.h>
#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
PSP_MODULE_INFO("Native Platinum application",0,0,1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4096);
extern void NitroMain(void);
/* SDK_PORT gx_load3d.c texture/palette LCDC base tables are zero until these run (desktop sim_main.cpp calls them) */
extern void WIN_Init_sTexStartAddrTable(void);
extern void WIN_Init_sTexPlttStartAddrTable(void);
extern void PSPNativeFrameInit(void);
extern BOOL PSPNativeOverlay_Init(void);
extern BOOL PSPNativeRomFS_SetPath(const char *path);
extern BOOL PSPNative_OpenBackup(const char *path);
static int exitCallback(int a,int b,void*c){sceKernelExitGame();return 0;}
static int callbackThread(SceSize args,void*argp){int cb=sceKernelCreateCallback("exit",exitCallback,NULL);sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}
int main(void){
 {int th=sceKernelCreateThread("cbthread",callbackThread,0x11,0xFA0,0,NULL);if(th>=0)sceKernelStartThread(th,0,NULL);}
 scePowerSetClockFrequency(333,333,166);
 if(!PSPNativeOverlay_Init())return 1;
 PSPNativeRomFS_SetPath("Platinum.nds");
 if(!PSPNative_OpenBackup("Platinum.native.sav")){printf("[NATIVE-APP] backup missing or invalid; refusing to modify another save\n");return 1;}
 printf("[NATIVE-APP] entering actual NitroMain, fixed30-update policy; offline networking initialization omitted\n");
 WIN_Init_sTexStartAddrTable();WIN_Init_sTexPlttStartAddrTable();
 CTRDGi_GetModuleInfoAddr()->moduleID.raw=0xffff; /* PSP has no GBA cartridge slot. */
 PSPNativeFrameInit();
 NitroMain();return 0;
}
