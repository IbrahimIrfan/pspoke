#include <stdint.h>
#include <stdio.h>
int main(void){unsigned checks=0;for(int r=-32768;r<=32767;r++)for(int v=0;v<128;v++){volatile int a=r,b=v;int16_t old=(int16_t)((double)a*((double)b/128.0));int16_t fresh=(int16_t)((a*b)/128);if(old!=fresh)return 1;checks++;}printf("scale checks=%u failures=0\n",checks);}
