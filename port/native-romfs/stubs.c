#include <nitro/types.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
void MI_CpuCopy8(const void*src,void*dst,u32 size){const u8*s=src;u8*d=dst;while(size--)*d++=*s++;}
void OS_Terminate(void){printf("[ROMFS] real boot.c sanity check FAILED\n");abort();}

void ErrorHandling_AssertFail(void){printf("[ROMFS] actual game assertion FAILED\n");abort();}
