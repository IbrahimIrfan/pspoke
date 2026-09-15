#include <nitro.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
extern void PSPNativeG3CommandsBorrowed(const void*,unsigned);
/* Native G3 dispatcher borrows command bytes synchronously. DMA's
   native completion is dispatch completion; it never touches Allegrex DMA regs. */
void MI_SendGXCommand(u32 dma,const void*src,u32 bytes){
 if(dma>3||(bytes&3)||((uintptr_t)src&3)||(!src&&bytes))abort();if(!bytes)return;
 PSPNativeG3CommandsBorrowed(src,bytes);
}
void MI_SendGXCommandFast(u32 dma,const void*src,u32 bytes){MI_SendGXCommand(dma,src,bytes);}
void MI_SendGXCommandAsync(u32 dma,const void*src,u32 bytes,MIDmaCallback cb,void*arg){MI_SendGXCommand(dma,src,bytes);if(cb)cb(arg);}
void MI_SendGXCommandAsyncFast(u32 dma,const void*src,u32 bytes,MIDmaCallback cb,void*arg){MI_SendGXCommandAsync(dma,src,bytes,cb,arg);}
