/* Pal Pad friend-code entry: clear its VBlank callback before its heap goes away.
 *
 * The friend-code entry application (unk_020850F4, translated assembly) installs sub_020855CC as the VBlank
 * callback in its init, and its exit sub_0208527C frees the BgConfig and destroys heap 0x6C without ever clearing
 * it. The next VBlank then ran DoScheduledBgGpuUpdates on the freed BgConfig and aborted in MI_DmaCopy16 with a
 * garbage size. The original game has the same bug; on the DS the freed memory still held the old data.
 *
 * The exit function is translated code, so the fix sits on the call it makes last: Heap_Destroy(0x6C). Only that
 * callback is cleared, so another screen's callback is never touched. */
#include <nitro.h>
#include "heap.h"
#include "system.h"

extern void sub_020855CC(void *);
extern void __real_Heap_Destroy(enum HeapID heapId);

void __wrap_Heap_Destroy(enum HeapID heapId)
{
    if (heapId == 0x6C && gSystem.vBlankIntr == (GFIntrCB)sub_020855CC) {
        Main_SetVBlankIntrCB(NULL, NULL);
    }
    __real_Heap_Destroy(heapId);
}
