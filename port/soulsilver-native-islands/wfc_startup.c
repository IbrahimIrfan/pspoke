/* Native C transcription of SoulSilver's two DWC initialization wrappers.
 * SDK implementation and native overlay residency services remain external. */
#include <stdint.h>
#include "heap.h"
extern int DWC_Init(void *alignedWork);
extern void LoadDwcOverlay(void), LoadOVY38(void);
extern void UnloadDwcOverlay(void), UnloadOVY38(void);
int sub_02039FD8(enum HeapID heapID)
{
    void *allocation=Heap_Alloc(heapID,0x720);
    void *aligned=(void *)(((uintptr_t)allocation+31u)&~(uintptr_t)31u);
    int result=DWC_Init(aligned);
    Heap_Free(allocation);
    return result;
}
int sub_02039FFC(enum HeapID heapID)
{
    LoadDwcOverlay();
    LoadOVY38();
    int result=sub_02039FD8(heapID);
    UnloadDwcOverlay();
    UnloadOVY38();
    return result;
}
