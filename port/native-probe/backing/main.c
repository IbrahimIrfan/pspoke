#include <pspkernel.h>
#include <pspdebug.h>
#include <stdint.h>
#include <stdio.h>
#include <nitro.h>
PSP_MODULE_INFO("Native backing proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(1024);
#include "alias_checks.h"
int main(void){
 unsigned failures=0,checks=0;
 for(unsigned i=0;i<sizeof(probes)/sizeof(probes[0]);i++)for(unsigned j=0;j<sizeof(probes)/sizeof(probes[0]);j++){
  if(probes[i].family!=probes[j].family)continue;
  int diff=(int)probes[j].offset-(int)probes[i].offset;
  if(diff < -4096 || diff >4096)continue;
  checks++;if((intptr_t)probes[j].address-(intptr_t)probes[i].address!=diff)failures++;
 }
 reg_CP_DIV_NUMER=0x1122334455667788ull;
 checks+=2;failures+=reg_CP_DIV_NUMER_L!=0x55667788u;failures+=reg_CP_DIV_NUMER_H!=0x11223344u;
 checks+=4;failures+=(uintptr_t)s_HW_OBJ_PLTT-(uintptr_t)s_HW_BG_PLTT!=512;failures+=(uintptr_t)s_HW_DB_BG_PLTT-(uintptr_t)s_HW_BG_PLTT!=1024;failures+=(uintptr_t)s_HW_DB_OBJ_PLTT-(uintptr_t)s_HW_BG_PLTT!=1536;failures+=HW_PLTT_SIZE!=1024;
 unsigned char *dtcm=(unsigned char*)SDK_AUTOLOAD_DTCM_START;dtcm[0]=0x12;dtcm[16383]=0x34;checks+=2;failures+=dtcm[0]!=0x12;failures+=dtcm[16383]!=0x34;
 printf("[BACKING] checks=%u failures=%u palette=%llu DTCM16K=ok\n",checks,failures,(unsigned long long)HW_PLTT_SIZE);
 pspDebugScreenInit();pspDebugScreenSetTextColor(failures?0xff0000ff:0xff00ff00);pspDebugScreenPrintf("Native backing: %s\n%u alias/memory checks\n",failures?"FAIL":"PASS",checks);
 sceKernelDelayThread(3000000);sceKernelExitGame();return failures!=0;
}
