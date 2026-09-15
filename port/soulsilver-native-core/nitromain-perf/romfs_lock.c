#include <pspkernel.h>
#include <stdlib.h>
#include <stdio.h>
static SceLwMutexWorkarea mutex;
static volatile int created;
void SSRomLock(void){
 /* Reserve initialization with interrupts masked, then perform the kernel
  * allocation with interrupts restored. Concurrent first callers wait. */
 for(;;){unsigned flags=sceKernelCpuSuspendIntr();int mine=created==0;int ready=created==1;if(mine)created=-1;sceKernelCpuResumeIntr(flags);
  if(ready)break;
  if(mine){int r=sceKernelCreateLwMutex(&mutex,"SS readonly ROM",PSP_LW_MUTEX_ATTR_RECURSIVE,0,NULL);if(r<0){printf("[SS-ROMFS] mutex create failed %08x\n",r);abort();}flags=sceKernelCpuSuspendIntr();created=1;sceKernelCpuResumeIntr(flags);break;}
  sceKernelDelayThread(1);
 }
 int r=sceKernelLockLwMutex(&mutex,1,NULL);if(r<0){printf("[SS-ROMFS] mutex lock failed %08x\n",r);abort();}
}
void SSRomUnlock(void){if(sceKernelUnlockLwMutex(&mutex,1)<0)abort();}
