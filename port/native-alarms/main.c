#include <nitro/os.h>
#include <pspkernel.h>
#include <stdio.h>
PSP_MODULE_INFO("Native alarm proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
extern BOOL PSPNativeAlarmInCallback(void);
OSTick OS_GetTick(void){u64 t=sceKernelGetSystemTimeWide();return(t/64000000)*OS_SYSTEM_CLOCK+(t%64000000)*OS_SYSTEM_CLOCK/64000000;}
static volatile int hits,bad;static OSTick fired;
static void handler(void*p){hits+=*(int*)p;fired=OS_GetTick();if(!PSPNativeAlarmInCallback())bad++;}
int main(void){int fail=0,value=1;OSAlarm a,b;OS_InitAlarm();OS_CreateAlarm(&a);OS_CreateAlarm(&b);OSTick before=OS_GetTick();OS_SetAlarm(&a,OS_MilliSecondsToTicks(10),handler,&value);sceKernelDelayThread(30000);if(hits!=1||a.handler||fired-before<OS_MilliSecondsToTicks(9))fail++;
OS_SetAlarm(&a,OS_MilliSecondsToTicks(10),handler,&value);OS_CancelAlarm(&a);sceKernelDelayThread(20000);if(hits!=1)fail++;
OS_SetAlarm(&a,OS_MilliSecondsToTicks(10),handler,&value);OS_SetAlarmTag(&a,7);OS_SetAlarm(&b,OS_MilliSecondsToTicks(10),handler,&value);OS_SetAlarmTag(&b,7);OS_CancelAlarms(7);sceKernelDelayThread(20000);if(hits!=1)fail++;
OS_SetPeriodicAlarm(&a,OS_GetTick(),OS_MilliSecondsToTicks(5),handler,&value);sceKernelDelayThread(27000);OS_CancelAlarm(&a);int count=hits;sceKernelDelayThread(15000);if(count<5||count>7||hits!=count)fail++;
OS_EndAlarm();if(OS_IsAlarmAvailable()||PSPNativeAlarmInCallback()||bad)fail++;printf("[ALARM] failures=%d hits=%d realPSP one-shot/cancel/tag/periodic\n",fail,hits);sceKernelExitGame();return fail;}
