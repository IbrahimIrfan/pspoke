#ifndef SS_TEXTURE_EQUAL_H
#define SS_TEXTURE_EQUAL_H
#include <stdint.h>
#include <string.h>
// Equality only: unlike memcmp, no first-mismatch ordering is required.
// Read exactly the requested range, including unaligned and short tails.
static __attribute__((noinline)) int SS_TextureBytesEqual(const void *a,const void *b,unsigned n){
 const unsigned char *x=(const unsigned char*)a,*y=(const unsigned char*)b;
 if(((uintptr_t)x|(uintptr_t)y)&3)return memcmp(x,y,n)==0;
 const uint32_t *wx=(const uint32_t*)x,*wy=(const uint32_t*)y;
 while(n>=32){
  uint32_t different=(wx[0]^wy[0])|(wx[1]^wy[1])|(wx[2]^wy[2])|(wx[3]^wy[3])|
                     (wx[4]^wy[4])|(wx[5]^wy[5])|(wx[6]^wy[6])|(wx[7]^wy[7]);
  if(different)return 0;wx+=8;wy+=8;n-=32;
 }
 while(n>=4){if(*wx++!=*wy++)return 0;n-=4;}
 x=(const unsigned char*)wx;y=(const unsigned char*)wy;
 while(n--){if(*x++!=*y++)return 0;}return 1;
}
#endif
