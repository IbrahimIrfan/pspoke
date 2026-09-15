#include <pspkernel.h>
#include <psppower.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
PSP_MODULE_INFO("SS native Thumb proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
#include "generated.c"
static unsigned allocations,clears,call_errors;
__attribute__((noinline)) void *Heap_Alloc(int heap,uint32_t size){allocations++;if(heap!=4||size!=0x48)call_errors++;void*p=malloc(size);if(p)memset(p,0xa5,size);return p;}
__attribute__((noinline)) void MIi_CpuClear32(uint32_t v,void*p,uint32_t n){clears++;if(v||n!=0x48||!p)call_errors++;for(unsigned i=0;i<n/4;i++)((uint32_t*)p)[i]=v;}
static uint32_t seed=0xC001BEEF;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static uint32_t pack(void*p){return(uint32_t)(uintptr_t)p;}
typedef struct{uint16_t*threshold;uint8_t*low;uint8_t*high;uint32_t n;} Table;
__attribute__((noinline)) static unsigned ref_lookup(Table*t,uint32_t q){unsigned i=0;while(i+1<t->n&&t->threshold[i+1]<=q)i++;return t->low[i]+256u*t->high[i];}
int main(void){
 unsigned failures=0,table_checks=0,init_checks=0,alloc_checks=0,flag_checks=0;uint32_t hash=0;
 union{uint32_t align[128];uint8_t bytes[512];} input;
 for(unsigned iter=0;iter<2048;iter++){
  uint32_t n=1+rnd()%64;memset(&input,0xcc,sizeof input);memcpy(input.bytes,&n,4);
  uint16_t*threshold=(uint16_t*)(input.bytes+4);uint8_t*low=input.bytes+4+2*n,*high=low+n;
  unsigned value=0;for(unsigned i=0;i<n;i++){value+=rnd()%500;threshold[i]=value;low[i]=rnd()>>16;high[i]=rnd()>>24;}
  struct{uint32_t before;Table t;uint32_t after;} guarded={0x12345678,{0},0x87654321};
  native_sub_02026E18(pack(input.bytes),pack(&guarded.t),0x33333333,0x44444444);
  Table*t=&guarded.t;init_checks++;
  if(t->threshold!=threshold||t->low!=low||t->high!=high||t->n!=n||guarded.before!=0x12345678||guarded.after!=0x87654321)failures++;
  for(unsigned j=0;j<256;j++){
   uint32_t q=j<3*n?((uint32_t)threshold[j/3]+j%3-1):rnd();
   uint32_t got=native_sub_02026DE0(pack(t),q,0xdeadcafe,0xabad1dea),want=ref_lookup(t,q);
   table_checks++;if(got!=want)failures++;hash=hash*31+got;
  }
 }
 for(unsigned i=0;i<10000;i++){
  void*p=(void*)(uintptr_t)native_ModelAttributes_Init(rnd(),rnd(),rnd(),rnd());
  if(!p){failures++;break;}for(unsigned j=0;j<72;j++){alloc_checks++;if(((uint8_t*)p)[j])failures++;}free(p);
 }
 /* Arithmetic flag helpers checked independently using signed/unsigned wide ranges. */
 uint32_t edge[]={0,1,0x7fffffff,0x80000000,0xffffffff,0x10000,0x40000000};
 for(unsigned i=0;i<100049;i++){
  uint32_t x=i<49?edge[i/7]:rnd(),y=i<49?edge[i%7]:rnd();Flags a={0},s={0};
  uint32_t ar=x+y,sr=x-y;af(&a,x,y,ar);sf(&s,x,y,sr);
  int64_t sa=(int64_t)(int32_t)x+(int32_t)y,ss=(int64_t)(int32_t)x-(int32_t)y;
  flag_checks+=8;
  failures+=(a.n!=(ar>=0x80000000u))+(a.z!=(ar==0))+(a.c!=((uint64_t)x+y>UINT32_MAX))+(a.v!=(sa>INT32_MAX||sa<INT32_MIN));
  failures+=(s.n!=(sr>=0x80000000u))+(s.z!=(sr==0))+(s.c!=(x>=y))+(s.v!=(ss>INT32_MAX||ss<INT32_MIN));
  unsigned n=i%256;Flags f={0,0,i&1,0};unsigned oldc=f.c;uint32_t got=shift(&f,x,n);
  uint64_t wide=n<32?((uint64_t)x<<n):0;uint32_t want=n<32?(uint32_t)wide:0;
  unsigned carry=n==0?oldc:n<32?(wide>>32)&1:n==32?x&1:0;
  flag_checks+=4;failures+=(got!=want)+(f.c!=carry)+(f.n!=(want>=0x80000000u))+(f.z!=(want==0));
 }
 failures+=call_errors+(allocations!=10000)+(clears!=10000);
 printf("[SS-NATIVE-CODEGEN] functions=3 assembly_instructions=50 init_checks=%u table_checks=%u allocation_byte_checks=%u flag_checks=%u allocations=%u clears=%u failures=%u hash=%08x\n",init_checks,table_checks,alloc_checks,flag_checks,allocations,clears,failures,hash);
 uint16_t bt[64];uint8_t bl[64],bh[64];for(unsigned i=0;i<64;i++){bt[i]=i*100;bl[i]=i*3;bh[i]=i*7;}
 Table bench={bt,bl,bh,64};uint32_t h1=0,h2=0;unsigned count=100000;
 scePowerSetClockFrequency(333,333,166);
 uint64_t start=sceKernelGetSystemTimeWide();for(unsigned i=0;i<count;i++)h1=h1*31+native_sub_02026DE0(pack(&bench),(i*117u)%6500,0,0);uint64_t mid=sceKernelGetSystemTimeWide();
 for(unsigned i=0;i<count;i++)h2=h2*31+ref_lookup(&bench,(i*117u)%6500);uint64_t end=sceKernelGetSystemTimeWide();
 printf("[SS-NATIVE-CODEGEN-BENCH] count=%u generated_us=%u reference_us=%u same_hash=%u hash=%08x\n",count,(unsigned)(mid-start),(unsigned)(end-mid),h1==h2,h1);
 sceKernelExitGame();return failures!=0||h1!=h2;
}
