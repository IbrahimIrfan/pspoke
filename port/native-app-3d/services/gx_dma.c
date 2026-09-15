#include <nitro.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
/* Native G3 dispatcher consumes copied command bytes synchronously. DMA's
   native completion is dispatch completion; it never touches Allegrex DMA regs. */
void MI_SendGXCommand(u32 dma,const void*src,u32 bytes){
 if(dma>3||(bytes&3)||((uintptr_t)src&3)||(!src&&bytes))abort();if(!bytes)return;
 draw_msg_t msg;memset(&msg,0,sizeof(msg));msg.type=DRAW_CMD_G3_CMD_LIST;msg.size=bytes;msg.data.ptr=malloc(bytes);if(!msg.data.ptr)abort();memcpy(msg.data.ptr,src,bytes);SIM_HandleG3Command(&msg);/* dispatcher owns and frees data.ptr */
}
void MI_SendGXCommandFast(u32 dma,const void*src,u32 bytes){MI_SendGXCommand(dma,src,bytes);}
void MI_SendGXCommandAsync(u32 dma,const void*src,u32 bytes,MIDmaCallback cb,void*arg){MI_SendGXCommand(dma,src,bytes);if(cb)cb(arg);}
void MI_SendGXCommandAsyncFast(u32 dma,const void*src,u32 bytes,MIDmaCallback cb,void*arg){MI_SendGXCommandAsync(dma,src,bytes,cb,arg);}
