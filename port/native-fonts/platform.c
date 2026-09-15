#include <nitro.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
u8 s_HW_MAIN_MEM[0x800000],s_HW_MAIN_MEM_SYSTEM[0x400];
#define arena s_HW_MAIN_MEM
#define ARENA_BYTES (6*1024*1024)
static uintptr_t lo,hi;
void OS_Init(void){lo=(uintptr_t)arena;hi=lo+ARENA_BYTES;printf("[STARTUP] OS arena initialized 6MiB in main backing\n");}
void *OS_AllocFromArenaLo(OSArenaId id,u32 size,u32 align){if(id!=OS_ARENA_MAIN||!align||(align&(align-1)))abort();uintptr_t p=(lo+align-1)&~(align-1);if(p+size>hi)return NULL;lo=p+size;return (void*)p;}
void *OS_AllocFromArenaHi(OSArenaId id,u32 size,u32 align){if(id!=OS_ARENA_MAIN||!align||(align&(align-1)))abort();uintptr_t p=(hi-size)&~(align-1);if(size>hi-lo||p<lo)return NULL;hi=p;return (void*)p;}
void OS_GetLowEntropyData(u32*data){u64 t=sceKernelGetSystemTimeWide();for(int i=0;i<8;i++)data[i]=(u32)(t>>(i&1?32:0))^(i*0x9e3779b9u);}

static uint64_t tickStartUs;
static OSTick tickBase;
static BOOL tickReady;
void OS_InitTick(void){tickStartUs=sceKernelGetSystemTimeWide();tickBase=0;tickReady=TRUE;}
BOOL OS_IsTickAvailable(void){return tickReady;}
OSTick OS_GetTick(void){
 uint64_t elapsed=sceKernelGetSystemTimeWide()-tickStartUs;
 // DS OS ticks use systemclock/64, not PSP microseconds. Split the quotient
 // to avoid multiplying a long-running microsecond count past64bits.
 return tickBase+(elapsed/64000000u)*OS_SYSTEM_CLOCK+((elapsed%64000000u)*OS_SYSTEM_CLOCK)/64000000u;
}
u16 OS_GetTickLo(void){return (u16)OS_GetTick();}
void OS_SetTick(OSTick value){tickStartUs=sceKernelGetSystemTimeWide();tickBase=value;tickReady=TRUE;}

static OSIntrMode intr;
OSIntrMode OS_DisableInterrupts(void){OSIntrMode old=intr;intr=1;return old;}
OSIntrMode OS_RestoreInterrupts(OSIntrMode mode){OSIntrMode old=intr;intr=mode;return old;}
static OSProcMode procMode = OS_PROCMODE_SYS;
OSProcMode OS_GetProcMode(void){return procMode;}
static OSIrqFunction callbacks[32];static OSIrqMask mask;
void OS_SetIrqFunction(OSIrqMask bits,OSIrqFunction fn){for(int i=0;i<32;i++)if(bits&(1u<<i))callbacks[i]=fn;}
OSIrqMask OS_EnableIrqMask(OSIrqMask bits){OSIrqMask old=mask;mask|=bits;return old;}
void SIM_handleAssertionFailureMsg(const char*file,unsigned line,const char*fmt,...){printf("[STARTUP] SDK assertion %s:%u ",file,line);va_list a;va_start(a,fmt);vprintf(fmt,a);va_end(a);abort();}
void ErrorHandling_AssertFail(void){printf("[STARTUP] game assertion\n");abort();}
void OS_Terminate(void){printf("[STARTUP] OS terminate\n");abort();}

u32 s_HW_INTR_CHECK_BUF;
BOOL CommManager_IsInitialized(void){printf("[STARTUP] unsupported allocation-failure networking branch\n");abort();}
void ErrorMessageReset_PrintErrorAndReset(void){printf("[STARTUP] unsupported reset branch\n");abort();}
static u32 cardIcacheThreshold,cardDcacheThreshold;
void CARD_SetCacheFlushThreshold(u32 icache,u32 dcache){cardIcacheThreshold=icache;cardDcacheThreshold=dcache;printf("[STARTUP] CARD cache thresholds %lu/%lu; synchronous stdio backend\n",(unsigned long)icache,(unsigned long)dcache);}

void OS_WaitIrq(BOOL clear,OSIrqMask bits){
 if(bits!=OS_IE_V_BLANK){printf("[STARTUP] unsupported IRQ wait %08lx\n",(unsigned long)bits);abort();}
 if(clear)s_HW_INTR_CHECK_BUF &= ~bits;
 sceDisplayWaitVblankStart();
 if(!intr && (mask & bits) && callbacks[0]){OSProcMode old=procMode;procMode=OS_PROCMODE_IRQ;callbacks[0]();procMode=old;}
}
