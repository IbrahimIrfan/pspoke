#define SS_FADE_WRAPPER_PROOF
#include "fade_wrapper.c"
#include <stdio.h>
#include <stdlib.h>
static unsigned calls,errors;static int result;
int SSProofDoFadeUpdateFrame(void *a,void *b,void *c){calls=calls*10+1;if(a!=ssdata_unk_0200FA24__021D0EF4||b!=ssdata_unk_0200FA24__021D0EF4+0x14||c!=ssdata_unk_0200FA24__021D0EF4+0x44)errors++;return result;}
void SSProofHandleEndFade(void *a){calls=calls*10+2;if(a!=ssdata_unk_0200FA24__021D0EF4)errors++;}
void SSNativeFadeProof(void){u16 *active=(u16*)(ssdata_unk_0200FA24__021D1034+12),old=*active;unsigned checks=0;
 const int outcomes[]={0,1,2,-1,0x7fffffff};for(unsigned k=0;k<65536;k++)for(unsigned j=0;j<5;j++){
  *active=k;calls=0;result=outcomes[j];SSProofHandleFadeUpdateFrame();unsigned expected=k?(result==1?12:1):0;
  checks++;if(calls!=expected)errors++;checks++;if(SSProofIsPaletteFadeFinished()!=(k==0))errors++;
 }
 *active=old;printf("[SS-FADE-WRAPPER] checks=%u errors=%u\n",checks,errors);if(errors)abort();}
