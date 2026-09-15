#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include "billboard.h"
PSP_MODULE_INFO("SS Billboard create proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
struct {BillboardList*lists;int count;} ssdata_unk_02023694__021D2208;
static unsigned checks,failures,expectedSize;static enum HeapID expectedHeap;static unsigned char arena[32+224*64+32] __attribute__((aligned(4)));
#define CHECK(x) do{checks++;if(!(x)){if(failures<8)printf("FAIL %d\n",__LINE__);failures++;}}while(0)
void GF_AssertFail(void){CHECK(0);}
void *Heap_Alloc(enum HeapID heap,u32 size){CHECK(heap==expectedHeap&&size==expectedSize);return arena+32;}
int main(void){
 for(int count=0;count<=64;count++)for(int heap=0;heap<128;heap++){
  unsigned char expected[sizeof arena];memset(arena,(count+heap)&255,sizeof arena);memcpy(expected,arena,sizeof arena);expectedSize=count*224;expectedHeap=heap;ssdata_unk_02023694__021D2208.lists=NULL;ssdata_unk_02023694__021D2208.count=-1;
  for(int i=0;i<count;i++){unsigned char*p=expected+32+i*224;p[0]=p[1]=p[3]=0;unsigned ofs[]={4,8,0xd0,0xd4,0xd8,0xdc};for(unsigned j=0;j<6;j++)memset(p+ofs[j],0,4);}
  BillboardLists_Create(count,heap);CHECK(ssdata_unk_02023694__021D2208.lists==(BillboardList*)(arena+32)&&ssdata_unk_02023694__021D2208.count==count);CHECK(!memcmp(expected,arena,sizeof arena));
 }
 printf("[SS-BILLBOARD-CREATE] checks=%u failures=%u\n",checks,failures);sceKernelExitGame();return failures!=0;}
