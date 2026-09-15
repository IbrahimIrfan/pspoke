#include <nitro.h>
#include <pspkernel.h>
#include <stdio.h>
#include "system.h"
#include "heap.h"
#include "sys_task_manager.h"
#include <stdint.h>
#include <string.h>
PSP_MODULE_INFO("Native Startup",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);

static unsigned ran,failures;
static void TaskCheck(SysTask*t,void*p){if(OS_GetProcMode()!=OS_PROCMODE_IRQ)failures++;ran++;SysTask_Delete(t);}
static unsigned CheckWords(const u32*p,unsigned n,u32 value){unsigned bad=0;for(unsigned i=0;i<n;i++)bad+=p[i]!=value;return bad;}
int main(void){
 printf("[STARTUP] enter InitSystem\n");InitSystem();printf("[STARTUP] InitSystem returned\n");
 memset((void*)(uintptr_t)HW_LCDC_VRAM,0xA5,HW_LCDC_VRAM_SIZE);
 memset((void*)(uintptr_t)HW_PLTT,0xA5,HW_PLTT_SIZE);memset((void*)(uintptr_t)HW_DB_PLTT,0xA5,HW_DB_PLTT_SIZE);
 InitVRAM();printf("[STARTUP] InitVRAM returned\n");
 failures+=CheckWords((void*)(uintptr_t)HW_LCDC_VRAM,HW_LCDC_VRAM_SIZE/4,0);
 failures+=CheckWords((void*)(uintptr_t)HW_PLTT,HW_PLTT_SIZE/4,0)+CheckWords((void*)(uintptr_t)HW_DB_PLTT,HW_DB_PLTT_SIZE/4,0);
 failures+=CheckWords((void*)(uintptr_t)HW_OAM,HW_OAM_SIZE/4,192)+CheckWords((void*)(uintptr_t)HW_DB_OAM,HW_DB_OAM_SIZE/4,192);
 if(!gSystem.mainTaskMgr||!gSystem.vBlankTaskMgr||!gSystem.postVBlankTaskMgr||!gSystem.printTaskMgr)abort();
 if(gSystem.mainTaskMgr->maxTasks!=160||gSystem.vBlankTaskMgr->maxTasks!=64||gSystem.postVBlankTaskMgr->maxTasks!=32||gSystem.printTaskMgr->maxTasks!=4)failures++;
 for(unsigned id=0;id<4;id++){if(id==HEAP_ID_DEBUG)continue;u32 before=HeapExp_FndGetTotalFreeSize(id);if(id!=HEAP_ID_DEBUG){void*p=Heap_Alloc(id,128);if(!p)abort();memset(p,0x5A,128);Heap_Free(p);if(HeapExp_FndGetTotalFreeSize(id)!=before)failures++;}printf("[STARTUP] heap%u free=%lu\n",id,(unsigned long)before);}
 FSFile f;FS_InitFile(&f);char magic[4];if(!FS_OpenFile(&f,"graphic/pl_font.narc")||FS_ReadFile(&f,magic,4)!=4||memcmp(magic,"NARC",4))failures++;FS_CloseFile(&f);
 SysTaskManager_AddTask(gSystem.vBlankTaskMgr,TaskCheck,NULL,1);unsigned previous=gSystem.frameCounter;OS_WaitIrq(TRUE,OS_IE_V_BLANK);
 if(ran!=1||gSystem.frameCounter!=previous+1||OS_GetProcMode()!=OS_PROCMODE_SYS)failures++;
 printf("[STARTUP] validation failures=%u VBlankTasks=%u frame=%u FS=%d\n",failures,ran,gSystem.frameCounter,FS_IsAvailable());sceKernelExitGame();return failures;
}
