/* Original Thumb tail calls expressed as typed native calls. */
#include <stdint.h>
extern void NNS_SndPlayerSetAllocatableChannel(int,uint32_t);
extern void NNS_SndCaptureStopReverb(int);
extern int NNS_SndCaptureGetCaptureType(void);
void GF_SndSetAllocatableChannelForBGMPlayer(uint32_t channels){NNS_SndPlayerSetAllocatableChannel(7,channels);}
void sub_02005910(int frames){NNS_SndCaptureStopReverb(frames);}
int sub_02005908(void){return NNS_SndCaptureGetCaptureType();}
