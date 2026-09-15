#ifndef PSP_NATIVE_RENDER_H
#define PSP_NATIVE_RENDER_H
#ifdef __cplusplus
extern "C" {
#endif
/* Begin before native G3 commands. Present waits for GPU, composites both DS
 * engines, presents physical screens, and closes the frame. Return0 succeeds;
 * negative means unsupported capture/FIFO/state and caller should stop. */
int PSPNativeRenderInit(void);
int PSPNativeRenderBegin(void);
int PSPNativeRenderPresent(void);
int PSPNativeRenderPresentNoWait(void); /* Game already owns frame pacing. */
void PSPNativeRenderSetInput(unsigned keys,int touchMode,int down,int x,int y);
void PSPNativeRenderShutdown(void);
void PSPNativeRenderGetTimings(unsigned *readbackUs,unsigned *software2DUs);
unsigned PSPNativeRenderLastDrawMask(void); /* engineA bit0, engineB bit1 */
unsigned PSPNativeRenderFrameCount(void);
#ifdef __cplusplus
}
#endif
#endif
