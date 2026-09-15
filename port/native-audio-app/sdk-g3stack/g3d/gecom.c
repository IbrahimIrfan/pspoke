#include <nnsys/g3d/gecom.h>

static volatile int NNS_G3dFlagGXDmaAsync = 0;

static NNSG3dGeBuffer * NNS_G3dGeBuffer = NULL;

BOOL NNS_G3dGeIsSendDLBusy (void)
{
    return NNS_G3dFlagGXDmaAsync;
}

BOOL NNS_G3dGeIsBufferExist (void)
{
    return (NNS_G3dGeBuffer != NULL);
}

void NNS_G3dGeSetBuffer (NNSG3dGeBuffer * p)
{
    NNS_G3D_NULL_ASSERT(p);

    if (NNS_G3dGeBuffer == NULL) {
        p->idx = 0;
        NNS_G3dGeBuffer = p;
    }
}

NNSG3dGeBuffer * NNS_G3dGeReleaseBuffer (void)
{
    NNSG3dGeBuffer * p;

    NNS_G3dGeFlushBuffer();

    p = NNS_G3dGeBuffer;
    NNS_G3dGeBuffer = NULL;

    return p;
}

static NNS_G3D_INLINE void sendNB (const void * src, void * dst, u32 szByte)
{
    MI_CpuSend32(src, dst, szByte);
}

void NNS_G3dGeFlushBuffer (void)
{
    if (NNS_G3dFlagGXDmaAsync) {
        NNS_G3dGeWaitSendDL();
    }

    if (NNS_G3dGeBuffer &&
        NNS_G3dGeBuffer->idx > 0) {
        #ifdef SDK_PORT
        draw_msg_t msgStorage; /* PSP opt: synchronous handler, never retained */
        draw_msg_t * msg = &msgStorage;
        msg->type = DRAW_CMD_G3_CMD_LIST;
        u8 * cmdBuf = malloc( (NNS_G3dGeBuffer->idx << 2) );
        memcpy(cmdBuf, NNS_G3dGeBuffer->data, NNS_G3dGeBuffer->idx << 2);
        msg->data.ptr = cmdBuf;
        msg->size = (NNS_G3dGeBuffer->idx << 2);
        SIM_HandleG3Command( msg );
        /* stack msg: no free */
        #else
        sendNB(&NNS_G3dGeBuffer->data[0], (void *)&reg_G3X_GXFIFO, NNS_G3dGeBuffer->idx << 2);
        NNS_G3dGeBuffer->idx = 0;
        #endif
    }
}

void NNS_G3dGeWaitSendDL (void)
{
    #ifndef SDK_PORT
    while (NNS_G3dFlagGXDmaAsync)
        ;
    #endif
}

BOOL NNS_G3dGeIsImmOK (void)
{
    return (NNS_G3dGeBuffer == NULL || NNS_G3dGeBuffer->idx == 0) &&
           !NNS_G3dGeIsSendDLBusy();
}

BOOL NNS_G3dGeIsBufferOK (u32 numWord)
{
    return (NNS_G3dGeBuffer != NULL) &&
           (NNS_G3dGeBuffer->idx + numWord <= NNS_G3D_SIZE_COMBUFFER);
}

static void simpleUnlock_ (void * arg)
{
    *((volatile int *)arg) = 0;
}

#ifdef NNS_G3D_USE_FASTGXDMA
static BOOL NNS_G3dFlagUseFastDma = TRUE;
#else
static BOOL NNS_G3dFlagUseFastDma = FALSE;
#endif

void NNS_G3dGeUseFastDma (BOOL cond)
{
    NNS_G3dFlagUseFastDma = (cond);
}

void NNS_G3dGeSendDL (const void * src, u32 szByte)
{
    NNS_G3D_NULL_ASSERT(src);
    NNS_G3D_ASSERT(szByte >= 4);

    if (szByte < 256 || GX_DMAID == GX_DMA_NOT_USE) {
        NNS_G3dGeBufferOP_N(*(const u32 *)src,
                            (const u32 *)src + 1,
                            (szByte >> 2) - 1);
    } else {
        NNS_G3dGeFlushBuffer();
        NNS_G3dFlagGXDmaAsync = 1;

        if (NNS_G3dFlagUseFastDma) {
            MI_SendGXCommandAsyncFast(GX_DMAID,
                                      src,
                                      szByte,
                                      &simpleUnlock_,
                                      (void *)&NNS_G3dFlagGXDmaAsync);
        } else {
            MI_SendGXCommandAsync(GX_DMAID,
                                  src,
                                  szByte,
                                  &simpleUnlock_,
                                  (void *)&NNS_G3dFlagGXDmaAsync);
        }
    }
}

void NNS_G3dGeBufferOP_N (u32 op, const u32 * args, u32 num)
{
    if (NNS_G3dGeBuffer) {
        if (NNS_G3dFlagGXDmaAsync) {
            if (NNS_G3dGeBuffer->idx + 1 + num <= NNS_G3D_SIZE_COMBUFFER) {
                NNS_G3dGeBuffer->data[NNS_G3dGeBuffer->idx++] = op;
                if (num > 0) {
                    MI_CpuCopyFast(args, &NNS_G3dGeBuffer->data[NNS_G3dGeBuffer->idx], num << 2);
                    NNS_G3dGeBuffer->idx += num;
                }

                return;
            }
        }

        if (NNS_G3dGeBuffer->idx != 0) {
            NNS_G3dGeFlushBuffer();
        } else {
            if (NNS_G3dFlagGXDmaAsync) {
                NNS_G3dGeWaitSendDL();
            }
        }
    } else {
        if (NNS_G3dFlagGXDmaAsync) {
            NNS_G3dGeWaitSendDL();
        }
    }

    #ifdef SDK_PORT
    draw_msg_t msgStorage; /* PSP opt: synchronous handler, never retained */
    draw_msg_t * msg = &msgStorage;
    msg->type = DRAW_CMD_G3_CMD_LIST;
#ifdef OPT_CMDLIST_BORROW
    /* PSP opt (opt-tex): the renderer processes command bytes synchronously and never retains or frees
       borrowed bytes (PSPNativeG3CommandsBorrowed, same path as MI_SendGXCommand), so the per-op list lives
       on the stack instead of a malloc the renderer frees (~560 malloc+free per frame at Jubilife). */
    (void)msg;
    {
        extern void PSPNativeG3CommandsBorrowed(const void *, unsigned);
        u32 bytes = (num << 2) + 4;
        if (num < 63) {
            u32 stackBuf[64];
            stackBuf[0] = op;
            memcpy(stackBuf + 1, args, num << 2);
#ifdef OPT_CMDLIST_BORROW_VERIFY
            /* test: the renderer must leave borrowed bytes untouched (it never writes, retains or frees them) */
            {
                static unsigned checked, bad;
                u32 copy[64];
                memcpy(copy, stackBuf, bytes);
                PSPNativeG3CommandsBorrowed(stackBuf, bytes);
                if (memcmp(copy, stackBuf, bytes) && bad++ < 20) printf("[BORROW-VERIFY] MISMATCH op=%08x bytes=%u\n", op, bytes);
                if (!(++checked % 500000)) printf("[BORROW-VERIFY] checked=%u mismatches=%u\n", checked, bad);
            }
#else
            PSPNativeG3CommandsBorrowed(stackBuf, bytes);
#endif
        } else {
            u8 * cmdBuf = malloc(bytes);
            *(u32*)cmdBuf = op;
            memcpy(cmdBuf+4, args, num << 2);
            PSPNativeG3CommandsBorrowed(cmdBuf, bytes);
            free(cmdBuf);
        }
    }
#else
    u8 * cmdBuf = malloc( (sizeof(u8) * num << 2) + sizeof(u32) );
    *(u32*)cmdBuf = op;
    memcpy(cmdBuf+4, args, num << 2);
    msg->data.ptr = cmdBuf;
    msg->size = (num << 2) + 4;
    SIM_HandleG3Command( msg );
    /* stack msg: no free */
#endif
    #else
    reg_G3X_GXFIFO = op;
    sendNB(args, (void *)&reg_G3X_GXFIFO, num << 2);
    #endif
}
