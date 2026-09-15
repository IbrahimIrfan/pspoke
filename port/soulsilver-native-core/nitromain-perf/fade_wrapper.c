#include <nitro.h>
#ifdef SS_FADE_WRAPPER_PROOF
#define HandleFadeUpdateFrame SSProofHandleFadeUpdateFrame
#define IsPaletteFadeFinished SSProofIsPaletteFadeFinished
#define DoFadeUpdateFrame SSProofDoFadeUpdateFrame
#define HandleEndFade SSProofHandleEndFade
#endif
extern unsigned char ssdata_unk_0200FA24__021D1034[];
extern unsigned char ssdata_unk_0200FA24__021D0EF4[];
extern int DoFadeUpdateFrame(void*,void*,void*);
extern void HandleEndFade(void*);
void HandleFadeUpdateFrame(void){
 unsigned char *state=ssdata_unk_0200FA24__021D0EF4;
 if(*(u16*)(ssdata_unk_0200FA24__021D1034+12)!=0)
  if(DoFadeUpdateFrame(state,state+0x14,state+0x44)==1)HandleEndFade(state);
}
BOOL IsPaletteFadeFinished(void){return *(u16*)(ssdata_unk_0200FA24__021D1034+12)==0;}
