#include <nitro/os.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdint.h>
PSP_MODULE_INFO("Native thread proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
static OSThreadQueue queue;static volatile int entered,finished,destructed,failures;static OSThread workers[2];
static void destruct(void*p){destructed+=(int)(intptr_t)p;}
static void work(void*p){int index=(int)(intptr_t)p;if(OS_GetCurrentThread()!=&workers[index])failures++;entered++;OS_SleepThread(&queue);finished++;}
static void waitFor(volatile int*p,int n){for(int i=0;i<1000&&*p<n;i++)OS_Sleep(1);if(*p!=n)failures++;}
int main(void){OS_InitThread();OSThread*mainThread=OS_GetCurrentThread();if(!mainThread||OS_GetNumberOfThread()!=1)failures++;OS_InitThreadQueue(&queue);OS_WakeupThreadDirect(mainThread);if(sceKernelCancelWakeupThread(mainThread->id)!=0)failures++;
if(OS_DisableScheduler()!=0||OS_DisableScheduler()!=1||OS_EnableScheduler()!=2||OS_EnableScheduler()!=1||OS_EnableScheduler()!=0)failures++;
for(int i=0;i<2;i++){OS_CreateThread(&workers[i],work,(void*)(intptr_t)i,NULL,16384,16+i);OS_SetThreadDestructor(&workers[i],destruct);OS_SetThreadParameter(&workers[i],(void*)1);if(OS_GetThread(workers[i].id)!=&workers[i])failures++;OS_WakeupThreadDirect(&workers[i]);}
waitFor(&entered,2);if(finished||queue.head!=&workers[0]||queue.tail!=&workers[1])failures++;
if(!OS_SetThreadPriority(&workers[1],18)||OS_GetThreadPriority(&workers[1])!=18||OS_SetThreadPriority(&workers[1],32))failures++;
OS_WakeupThread(&queue);for(int i=0;i<2;i++)OS_JoinThread(&workers[i]);if(finished!=2||destructed!=2||queue.head||queue.tail||OS_GetNumberOfThread()!=1)failures++;
/* Reuse the same SDK object after native stack/thread reclamation. */
entered=finished=0;OS_CreateThread(&workers[0],work,NULL,NULL,16384,16);OS_WakeupThreadDirect(&workers[0]);waitFor(&entered,1);OS_WakeupThreadDirect(&workers[0]);OS_JoinThread(&workers[0]);if(finished!=1||queue.head||!OS_IsThreadTerminated(&workers[0]))failures++;
printf("[THREAD] failures=%d queue/reuse/priority/destructor/scheduler tested live=%d\n",failures,OS_GetNumberOfThread());sceKernelExitGame();return failures;}
