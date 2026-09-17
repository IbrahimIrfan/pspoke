#include <nitro.h>
#include <stdio.h>
#include <stdlib.h>
/* PSP has no DS lid; maintain actual SDK callback registrations. Entering DS
   sleep/power-off is unsupported and stops explicitly instead of succeeding. */
static PMSleepCallbackInfo *before,*after;
static BOOL ready;
void PM_Init(void){if(!ready){before=after=NULL;ready=TRUE;}}
static void Insert(PMSleepCallbackInfo**list,PMSleepCallbackInfo*info,BOOL append){if(!info||!info->callback)abort();for(PMSleepCallbackInfo*p=*list;p;p=p->next)if(p==info)abort();if(append){while(*list)list=&(*list)->next;info->next=NULL;*list=info;}else{info->next=*list;*list=info;}}
static void Delete(PMSleepCallbackInfo**list,PMSleepCallbackInfo*info){while(*list&&*list!=info)list=&(*list)->next;if(*list){*list=info->next;info->next=NULL;}}
void PM_PrependPreSleepCallback(PMSleepCallbackInfo*i){Insert(&before,i,FALSE);}
void PM_AppendPostSleepCallback(PMSleepCallbackInfo*i){Insert(&after,i,TRUE);}
void PM_DeletePreSleepCallback(PMSleepCallbackInfo*i){Delete(&before,i);}
void PM_DeletePostSleepCallback(PMSleepCallbackInfo*i){Delete(&after,i);}
void PM_GoSleepMode(PMWakeUpTrigger trigger,PMLogic logic,u16 keys){(void)trigger;(void)logic;(void)keys;puts("[NATIVE] DS sleep request unsupported on PSP");abort();}
u32 PM_ForceToPowerOff(void){puts("[NATIVE] DS power-off request unsupported on PSP");abort();}

/* PSP sleep mode. Memory Stick file handles do not survive suspend/resume: the next read from the ROM
   or write to the save fails, which froze the game on the first step after waking. A power callback
   counts completed resumes; the ROM reader and the save writer reopen their file when the count moved. */
#include <pspkernel.h>
#include <psppower.h>
static volatile unsigned resumeGen;
static int PowerCallback(int unknown,int flags,void*common){(void)unknown;(void)common;if(flags&PSP_POWER_CB_RESUME_COMPLETE)resumeGen++;return 0;}
void PSPNativeSuspendInit(void){int cb=sceKernelCreateCallback("pspoke_power",PowerCallback,NULL);if(cb>=0)scePowerRegisterCallback(0,cb);}
int PSPNativeResumedSince(unsigned*seen){unsigned g=resumeGen;if(*seen==g)return 0;*seen=g;return 1;}
