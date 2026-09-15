#include <nitro.h>
#include <stdio.h>
#include <string.h>
/* PSP has no physical Slot-2 bus or ARM7 cartridge service. Publish the same
 * absent-cartridge state consumed by the unmodified SDK query functions. */
void __wrap_CTRDG_Init(void){
 static BOOL initialized;
 if(initialized)return;
 CTRDGModuleInfo *info=CTRDGi_GetModuleInfoAddr();
 memset(info,0,sizeof(*info));info->moduleID.raw=0xffff;
 *(vu8*)HW_IS_CTRDG_EXIST=0;
 *(vu8*)HW_SET_CTRDG_MODULE_INFO_ONCE=1;
 initialized=TRUE;
 printf("[SLOT2] PSP has no GBA cartridge slot\n");
}
