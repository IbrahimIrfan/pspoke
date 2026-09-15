#include <pspkernel.h>
#include <pspdebug.h>
#include <stdint.h>
#include <stdio.h>
#include "expected.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <nitro/types.h>
#include "sys_task_manager.h"
#include <nnsys/fnd/expheap.h>
#include <string.h>
PSP_MODULE_INFO("Native Platinum core",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
extern void LCRNG_SetSeed(uint32_t seed);
extern uint32_t LCRNG_GetSeed(void);
extern uint16_t LCRNG_Next(void);
extern const uint32_t gTrainerAITable[],gTrainerAITableEnd[];
static unsigned order;
static void TaskProbe(SysTask *task,void *param){order=order*10u+(unsigned)(uintptr_t)param;SysTask_Delete(task);}
void SIM_handleAssertionFailureMsg(const char *file,unsigned line,const char *fmt,...){
 printf("[NATIVE-CORE] SDK ASSERT %s:%u ",file,line);va_list args;va_start(args,fmt);vprintf(fmt,args);va_end(args);abort();
}
void ErrorHandling_AssertFail(void){printf("[NATIVE-CORE] ASSERT FAIL\n");abort();}
int main(void) {
 unsigned failures=0,check=0;
 LCRNG_SetSeed(0x12345678u);
 for(unsigned i=0;i<1000;i++)check=check*31u+LCRNG_Next();
 if(check!=EXPECTED_CHECK||LCRNG_GetSeed()!=EXPECTED_SEED)failures++;
 unsigned words=((uintptr_t)gTrainerAITableEnd-(uintptr_t)gTrainerAITable)/4;
 if(words<=32||gTrainerAITable[0]!=32)failures++;
 for(unsigned i=0;i<32;i++)if(gTrainerAITable[i]<32||gTrainerAITable[i]>=words)failures++;
 void *heapmem=memalign(32,65536);
 if(!heapmem)abort();
 NNSFndHeapHandle heap=NNS_FndCreateExpHeapEx(heapmem,65536,0);
 if(!heap)abort();
 unsigned initialfree=NNS_FndGetTotalFreeSizeForExpHeap(heap);
 void *ha=NNS_FndAllocFromExpHeapEx(heap,257,32),*hb=NNS_FndAllocFromExpHeapEx(heap,1024,-32);
 if(!ha||!hb)abort();
 if(((uintptr_t)ha&31)||((uintptr_t)hb&31))failures++;
 memset(ha,0xA5,257);memset(hb,0x5A,1024);
 for(unsigned i=0;i<257;i++)if(((unsigned char*)ha)[i]!=0xA5)failures++;
 for(unsigned i=0;i<1024;i++)if(((unsigned char*)hb)[i]!=0x5A)failures++;
 NNS_FndFreeToExpHeap(heap,ha);NNS_FndFreeToExpHeap(heap,hb);
 unsigned finalfree=NNS_FndGetTotalFreeSizeForExpHeap(heap);
 if(initialfree!=finalfree)failures++;
 NNS_FndDestroyExpHeap(heap);free(heapmem);
 printf("[NATIVE-HEAP] initial=%u recovered=%u\n",initialfree,finalfree);
 unsigned taskbytes=SysTaskManager_GetRequiredSize(4);
 void *taskmem=calloc(1,taskbytes);
 if(!taskmem)abort();
 SysTaskManager *manager=SysTaskManager_Init(4,taskmem);
 SysTaskManager_AddTask(manager,TaskProbe,(void*)3,30);
 SysTaskManager_AddTask(manager,TaskProbe,(void*)1,10);
 SysTaskManager_AddTask(manager,TaskProbe,(void*)2,20);
 SysTaskManager_ExecuteTasks(manager);
 if(order!=123||manager->stackPointer!=0)failures++;
 SysTaskManager_AddTask(manager,TaskProbe,(void*)4,40);
 SysTaskManager_ExecuteTasks(manager);
 if(order!=1234||manager->stackPointer!=0)failures++;
 printf("[NATIVE-TASK] order=%u live=%u storage=%u\n",order,manager->stackPointer,taskbytes);
 free(taskmem);
 printf("[NATIVE-CORE] %s failures=%u seed=%08x check=%08x AIwords=%u first=%u\n",failures?"FAIL":"PASS",failures,(unsigned)LCRNG_GetSeed(),check,words,(unsigned)gTrainerAITable[0]);
 pspDebugScreenInit();
 pspDebugScreenPrintf("Native Platinum component proof\n\nReal game RNG: %s\nTrainer AI data: %u words\nFailures: %u\n\nNot a game boot or performance result.\n",check==EXPECTED_CHECK?"PASS":"FAIL",words,failures);
 sceKernelDelayThread(2000000);
 sceKernelExitGame();return failures;
}
