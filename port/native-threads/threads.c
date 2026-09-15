/* Native PSP thread lifecycle. DS stacks remain data; PSP owns native stacks. */
#include <nitro/os.h>
#include <pspkernel.h>
#include <pspintrman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef struct {OSThread *t; SceUID id; void(*fn)(void*); void *arg; BOOL started;} Record;
static Record records[32];static OSThread launcher;static BOOL ready;
static u32 schedulerDepth;static int dispatchState;
OSThreadInfo OSi_ThreadInfo;
static void check(int result,const char *op){if(result<0){printf("[THREAD] %s failed %08x\n",op,result);abort();}}
static Record *find(const OSThread*t){for(int i=0;i<32;i++)if(records[i].t==t)return &records[i];return NULL;}
void OS_InitThread(void){if(ready)return;memset(&launcher,0,sizeof(launcher));launcher.id=sceKernelGetThreadId();launcher.priority=16;launcher.state=OS_THREAD_STATE_READY;records[0]=(Record){&launcher,(SceUID)launcher.id,NULL,NULL,TRUE};OSi_ThreadInfo.list=OSi_ThreadInfo.current=&launcher;ready=TRUE;}
BOOL OS_IsThreadAvailable(void){return ready;}
OSThread *WIN_OS_GetCurrentThread(void){OS_InitThread();SceUID id=sceKernelGetThreadId();for(int i=0;i<32;i++)if(records[i].t&&records[i].id==id)return records[i].t;printf("[THREAD] unregistered caller %d\n",id);abort();}
static void dequeue(OSThread*t){OSThreadQueue*q=t->queue;if(!q)return;if(t->link.prev)t->link.prev->link.next=t->link.next;else q->head=t->link.next;if(t->link.next)t->link.next->link.prev=t->link.prev;else q->tail=t->link.prev;t->queue=NULL;t->link.prev=t->link.next=NULL;}
void OS_ExitThread(void){OSThread*t=WIN_OS_GetCurrentThread();if(t->destructor)t->destructor(t->userParameter);t->state=OS_THREAD_STATE_TERMINATED;sceKernelExitThread(0);abort();}
static int entry(SceSize n,void*a){(void)n;Record*r=*(Record**)a;r->t->state=OS_THREAD_STATE_READY;r->fn(r->arg);OS_ExitThread();return 0;}
void OS_CreateThreadReal(OSThread*t,void(*fn)(void*),void*arg,void*stack,u32 size,u32 priority){(void)stack;OS_InitThread();if(!t||!fn||priority>31||find(t))abort();Record*r=NULL;for(int i=1;i<32;i++)if(!records[i].t){r=&records[i];break;}if(!r)abort();memset(t,0,sizeof(*t));t->priority=priority;t->state=OS_THREAD_STATE_WAITING;t->context.func=(u64)(uintptr_t)fn;t->context.arg=(u64)(uintptr_t)arg;r->id=sceKernelCreateThread("PlatinumNative",entry,0x20+priority,size<16384?16384:size,PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU,NULL);check(r->id,"create");r->t=t;r->fn=fn;r->arg=arg;r->started=FALSE;t->id=r->id;t->next=OSi_ThreadInfo.list;OSi_ThreadInfo.list=t;}
void OS_CreateThreadDebug(OSThread*t,void(*fn)(void*),void*a,void*s,u32 n,u32 p,const char*name,const char*file,u32 line){(void)name;(void)file;(void)line;OS_CreateThreadReal(t,fn,a,s,n,p);}
void OS_WakeupThreadDirect(OSThread*t){Record*r=find(t);if(!r||t->state==OS_THREAD_STATE_TERMINATED)return;unsigned flags=sceKernelCpuSuspendIntr();if(r->started&&t->state!=OS_THREAD_STATE_WAITING){sceKernelCpuResumeIntr(flags);return;}dequeue(t);t->state=OS_THREAD_STATE_READY;BOOL start=!r->started;r->started=TRUE;sceKernelCpuResumeIntr(flags);if(start)check(sceKernelStartThread(r->id,sizeof(r),&r),"start");else check(sceKernelWakeupThread(r->id),"wake");}
void OS_SleepThread(OSThreadQueue*q){OSThread*t=WIN_OS_GetCurrentThread();if(schedulerDepth)abort();unsigned flags=sceKernelCpuSuspendIntr();if(t->queue)abort();if(q){t->queue=q;t->link.prev=q->tail;t->link.next=NULL;if(q->tail)q->tail->link.next=t;else q->head=t;q->tail=t;}t->state=OS_THREAD_STATE_WAITING;sceKernelCpuResumeIntr(flags);/* PSP wake counts preserve a wake between enqueue and sleep. */check(sceKernelSleepThread(),"sleep");t->state=OS_THREAD_STATE_READY;}
void OS_SleepThreadDirect(OSThread*t,OSThreadQueue*q){if(t!=WIN_OS_GetCurrentThread())abort();OS_SleepThread(q);}
void OS_WakeupThread(OSThreadQueue*q){if(!q)return;while(q->head)OS_WakeupThreadDirect(q->head);}
void OS_JoinThread(OSThread*t){Record*r=find(t);if(!r)return;if(!r->started||t==WIN_OS_GetCurrentThread())abort();check(sceKernelWaitThreadEnd(r->id,NULL),"join");check(sceKernelDeleteThread(r->id),"delete");OSThread**p=&OSi_ThreadInfo.list;while(*p&&*p!=t)p=&(*p)->next;if(*p)*p=t->next;memset(r,0,sizeof(*r));}
BOOL OS_IsThreadTerminated(const OSThread*t){return t&&t->state==OS_THREAD_STATE_TERMINATED;}
BOOL OS_SetThreadPriority(OSThread*t,u32 p){Record*r=find(t);if(!r||p>31)return FALSE;int result=sceKernelChangeThreadPriority(r->id,0x20+p);if(result<0)return FALSE;t->priority=p;return TRUE;}
u32 OS_GetThreadPriority(const OSThread*t){return t->priority;}
void OS_Sleep(u32 ms){if(schedulerDepth)abort();while(ms){u32 part=ms>1000000?1000000:ms;check(sceKernelDelayThread(part*1000),"delay");ms-=part;}}
void OS_YieldThread(void){if(!schedulerDepth)check(sceKernelRotateThreadReadyQueue(0),"yield");}
void OS_RescheduleThread(void){OS_YieldThread();}
u32 OS_DisableScheduler(void){u32 old=schedulerDepth;if(!old)dispatchState=sceKernelSuspendDispatchThread();schedulerDepth++;return old;}
u32 OS_EnableScheduler(void){u32 old=schedulerDepth;if(old&&!--schedulerDepth)check(sceKernelResumeDispatchThread(dispatchState),"resume dispatch");return old;}
OSThread* OS_GetThread(u32 id){for(OSThread*t=OSi_ThreadInfo.list;t;t=t->next)if(t->id==id)return t;return NULL;}
int OS_GetNumberOfThread(void){int n=0;for(OSThread*t=OSi_ThreadInfo.list;t;t=t->next)n++;return n;}
BOOL OS_IsThreadInList(const OSThread*t){return find(t)!=NULL;}
void OS_SetThreadDestructor(OSThread*t,OSThreadDestructor d){t->destructor=d;}
OSThreadDestructor OS_GetThreadDestructor(const OSThread*t){return t->destructor;}
void OS_SetThreadParameter(OSThread*t,void*p){t->userParameter=p;}
void *OS_GetThreadParameter(const OSThread*t){return t->userParameter;}
