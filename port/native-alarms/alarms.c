#include <nitro/os.h>
#include <pspkernel.h>
#include <pspintrman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {OSAlarm *alarm;SceUID id;} Slot;
static Slot slots[32];static BOOL ready;static volatile BOOL inCallback;
BOOL PSPNativeAlarmInCallback(void){return inCallback;}
void OS_InitAlarm(void){ready=TRUE;}
BOOL OS_IsAlarmAvailable(void){return ready;}
void OS_CreateAlarm(OSAlarm*a){memset(a,0,sizeof(*a));}
static Slot*find(OSAlarm*a){for(int i=0;i<32;i++)if(slots[i].alarm==a)return &slots[i];return NULL;}
static unsigned delay(OSTick ticks){/* Ceil conversion; cap long waits and recheck absolute fire time. */
 if(ticks>0xFFFFFFFFULL*OS_SYSTEM_CLOCK/64000000ULL)return 0xFFFFFFFFu;
 u64 us=(ticks*64000000ULL+OS_SYSTEM_CLOCK-1)/OS_SYSTEM_CLOCK;return us?us:1;
}
static SceUInt fire(void*p){Slot*s=p;OSAlarm*a=s->alarm;if(!a)return 0;OSTick now=OS_GetTick();if(now<a->fire)return delay(a->fire-now);
 SceUID original=s->id;OSAlarmHandler fn=a->handler;void*arg=a->arg;OSTick period=a->period;
 if(!period){a->handler=NULL;s->alarm=NULL;}else{do{a->fire+=period;}while(a->fire<=now);}
 inCallback=TRUE;fn(arg);inCallback=FALSE;
 if(!period||s->alarm!=a||s->id!=original||!a->handler)return 0;
 now=OS_GetTick();return delay(a->fire>now?a->fire-now:1);
}
void OS_SetAlarm(OSAlarm*a,OSTick tick,OSAlarmHandler fn,void*arg){if(!ready||!a||!fn)abort();unsigned flags=sceKernelCpuSuspendIntr();if(find(a))abort();Slot*s=NULL;for(int i=0;i<32;i++)if(!slots[i].alarm){s=&slots[i];break;}if(!s)abort();a->handler=fn;a->arg=arg;a->period=0;a->fire=OS_GetTick()+tick;s->alarm=a;s->id=sceKernelSetAlarm(delay(tick),fire,s);if(s->id<0){printf("[ALARM] set failed %08x\n",s->id);abort();}sceKernelCpuResumeIntr(flags);}
void OS_CancelAlarm(OSAlarm*a){unsigned flags=sceKernelCpuSuspendIntr();Slot*s=find(a);if(s){int result=sceKernelCancelAlarm(s->id);if(result<0){printf("[ALARM] cancel failed %08x\n",result);abort();}s->alarm=NULL;}a->handler=NULL;a->period=0;sceKernelCpuResumeIntr(flags);}
void OS_SetPeriodicAlarm(OSAlarm*a,OSTick start,OSTick period,OSAlarmHandler fn,void*arg){if(!period)abort();unsigned flags=sceKernelCpuSuspendIntr();OSTick now=OS_GetTick();OSTick next=start;if(next<=now)next+=((now-next)/period+1)*period;OS_SetAlarm(a,next-now,fn,arg);a->start=start;a->period=period;a->fire=next;sceKernelCpuResumeIntr(flags);}
void OS_SetAlarmTag(OSAlarm*a,u32 tag){if(!tag)abort();a->tag=tag;}
void OS_CancelAlarms(u32 tag){if(!tag)abort();for(int i=0;i<32;i++)if(slots[i].alarm&&slots[i].alarm->tag==tag)OS_CancelAlarm(slots[i].alarm);}
void OS_CancelAllAlarms(void){for(int i=0;i<32;i++)if(slots[i].alarm)OS_CancelAlarm(slots[i].alarm);}
void OS_EndAlarm(void){OS_CancelAllAlarms();ready=FALSE;}
