#include <nitro.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
PSP_MODULE_INFO("Native wordfill test",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
void SIM_handleAssertionFailureMsg(const char *f,unsigned l,const char*fmt,...){printf("[NATIVE-MEM] ASSERT %s:%u\n",f,l);abort();}
int main(void){unsigned failures[3]={0},checks=0;u32 words[18];
const u32 patterns[]={0,0xC0,0x12345678,0xFFFF0000,0x80000001,0xAABBCCDD};
for(unsigned p=0;p<6;p++)for(unsigned n=0;n<=16;n++)for(unsigned kind=0;kind<3;kind++){
 for(unsigned i=0;i<18;i++)words[i]=0xDEADBEEF;
 if(kind==0)MIi_CpuClear32(patterns[p],words+1,n*4);
 else if(kind==1)MI_DmaFill32(0,words+1,patterns[p],n*4);
 else MI_CpuFillFast(words+1,patterns[p],n*4);
 for(unsigned i=0;i<18;i++){u32 expected=i>=1&&i<n+1?patterns[p]:0xDEADBEEF;if(words[i]!=expected)failures[kind]++;checks++;}
}
printf("[NATIVE-MEM] checks=%u clear32=%u dma32=%u fast=%u\n",checks,failures[0],failures[1],failures[2]);sceKernelExitGame();return 0;}
