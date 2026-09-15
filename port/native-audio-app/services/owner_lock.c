#include <nitro.h>
#include <pspkernel.h>
#include <pspwlan.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Native PSP has no DS firmware profile. This is the baked English profile,
   matching the emulator's generated firmware policy; no settings UI required. */
void OS_GetOwnerInfo(OSOwnerInfo*p){if(!p)abort();memset(p,0,sizeof(*p));p->language=1;p->birthday.month=1;p->birthday.day=1;p->nickName[0]='P';p->nickName[1]='S';p->nickName[2]='P';p->nickNameLength=3;}
s64 OS_GetOwnerRtcOffset(void){return 0;} /* RTC adapter already reports PSP local time. */
void OS_GetMacAddress(u8*p){if(!p||sceWlanGetEtherAddr(p)<0)abort();}
static uint64_t ids;
s32 OS_GetLockID(void){unsigned saved=sceKernelCpuSuspendIntr();for(unsigned i=0;i<48;i++)if(!(ids&(UINT64_C(1)<<i))){ids|=UINT64_C(1)<<i;sceKernelCpuResumeIntr(saved);return OS_MAINP_LOCK_ID_START+i;}sceKernelCpuResumeIntr(saved);return OS_LOCK_ID_ERROR;}
void OS_ReleaseLockID(u16 id){if(id<0x40||id>0x6f)abort();unsigned saved=sceKernelCpuSuspendIntr();uint64_t bit=UINT64_C(1)<<(id-0x40);if(!(ids&bit))abort();ids&=~bit;sceKernelCpuResumeIntr(saved);}
u16 OS_ReadOwnerOfLockWord(OSLockWord*p){if(!p)abort();return p->ownerID;}
s32 OS_TryLockCartridge(u16 id){if(id<0x40||id>0x7f)return OS_LOCK_ERROR;OSLockWord*p=(OSLockWord*)HW_CTRDG_LOCK_BUF;unsigned saved=sceKernelCpuSuspendIntr();s32 prior=p->lockFlag;if(!prior){p->lockFlag=id;p->ownerID=id;}sceKernelCpuResumeIntr(saved);return prior;}
s32 OS_LockCartridge(u16 id){s32 result;while((result=OS_TryLockCartridge(id))>0)sceKernelDelayThread(100);return result;}
s32 OS_UnlockCartridge(u16 id){OSLockWord*p=(OSLockWord*)HW_CTRDG_LOCK_BUF;unsigned saved=sceKernelCpuSuspendIntr();if(p->ownerID!=id){sceKernelCpuResumeIntr(saved);return OS_UNLOCK_ERROR;}p->ownerID=0;p->lockFlag=0;sceKernelCpuResumeIntr(saved);return OS_UNLOCK_SUCCESS;}
s32 OS_UnLockCartridge(u16 id){return OS_UnlockCartridge(id);}
