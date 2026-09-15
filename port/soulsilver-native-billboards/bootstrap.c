#include "billboard.h"
#include "heap.h"
extern struct {BillboardList*lists;int count;} ssdata_unk_02023694__021D2208;
_Static_assert(sizeof(BillboardList)==0xe0,"SS billboard list size");
_Static_assert(sizeof(Billboard)==0xc4,"SS billboard size");
void BillboardLists_Create(int count,enum HeapID heap){
 GF_ASSERT(ssdata_unk_02023694__021D2208.lists==NULL);
 BillboardList *lists=Heap_Alloc(heap,sizeof(BillboardList)*count);
 ssdata_unk_02023694__021D2208.lists=lists;ssdata_unk_02023694__021D2208.count=count;
 for(int i=0;i<count;i++){
  BillboardList*l=&lists[i];l->active=0;l->draw=0;l->billboards=NULL;l->capacity=0;l->freeBillboards=NULL;l->freeBillboardHead=0;l->allocator=NULL;l->vramTransfer=NULL;l->redraw=0;
 }
}
