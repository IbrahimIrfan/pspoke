#include <nitro.h>
#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
PSP_MODULE_INFO("Native Platinum application",0,0,1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
#ifndef PSP_NATIVE_STACK_KB
#define PSP_NATIVE_STACK_KB 1024   /* pspsdk default is 256 KB; the whole DS game runs on this thread */
#endif
PSP_MAIN_THREAD_STACK_SIZE_KB(PSP_NATIVE_STACK_KB);
/* Malloc heap. The game's own allocator runs out of the 6 MiB static arena in
 * platform.c, so this heap serves the renderer (tile atlases up to 1.5 MiB plus
 * a 2 MiB 3D texture cache = 3.5 MiB worst case) and newlib. 4096 left only
 * 0.5 MiB of margin above that theoretical peak; measured free memory outside
 * the heap on a 64 MiB PSP is ~18.3 MiB, so 8192 is affordable and leaves the
 * partition with >14 MiB. Overridable for failure-injection tests. */
#ifndef PSP_NATIVE_HEAP_KB
#define PSP_NATIVE_HEAP_KB 8192
#endif
PSP_HEAP_SIZE_KB(PSP_NATIVE_HEAP_KB);
extern void NitroMain(void);
extern void PSPNativeStackProbeInit(void);
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
 {
  extern void PSPNativeMemLog(const char*,...);
  int before=sceKernelMaxFreeMemSize(),total=sceKernelTotalFreeMemSize();
  void *probe=malloc(16);
  int after=sceKernelMaxFreeMemSize();
  PSPNativeMemLog("[MEM] startup heap_kb=%d partition_maxfree_before=%d total_before=%d "
                  "partition_maxfree_after_first_malloc=%d heap_block_taken=%d malloc_ok=%d",
                  (int)PSP_NATIVE_HEAP_KB,before,total,after,before-after,probe?1:0);
  free(probe);
 }
 PSPNativeStackProbeInit();
 printf("[NATIVE-APP] entering actual NitroMain, fixed30-update policy; offline networking initialization omitted\n");
 WIN_Init_sTexStartAddrTable();WIN_Init_sTexPlttStartAddrTable();
 /* PSP has no GBA slot; let the real SDK take its absent-cartridge path. */
 CTRDG_Init();
 PSPNativeFrameInit();
 NitroMain();return 0;
}
