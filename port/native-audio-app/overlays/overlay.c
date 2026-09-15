#include <nitro.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* printf goes nowhere on a real PSP; the card log is the only evidence there. */
extern void PSPNativeMemLog(const char*fmt,...);
struct Range {unsigned char *data,*dataEnd,*bss,*bssEnd,*sinit,*sinitEnd;};
#include "ranges.h"
#include "rom_bounds.h"
#define COUNT (sizeof(ranges)/sizeof(ranges[0]))
static unsigned char *initialData[COUNT],state[COUNT];
static BOOL initialized;
BOOL PSPNativeOverlay_Init(void){
 if(initialized)return TRUE;
 for(unsigned i=0;i<COUNT;i++){
  size_t n=ranges[i].dataEnd-ranges[i].data;
  if(n){initialData[i]=malloc(n);if(!initialData[i]){for(unsigned j=0;j<i;j++){free(initialData[j]);initialData[j]=NULL;}return FALSE;}memcpy(initialData[i],ranges[i].data,n);}
 }
 initialized=TRUE;return TRUE;
}
BOOL FS_LoadOverlayInfo(FSOverlayInfo *o,MIProcessor target,FSOverlayID id){
 if(!o||target!=MI_PROCESSOR_ARM9||id>=COUNT)return FALSE;
 memset(o,0,sizeof(*o));o->target=target;o->header.id=id;
 /* DS addresses are used only as lifetime/overlap metadata, never dereferenced. */
 o->header.ram_address=(u8*)(uintptr_t)rom_bounds[id][1];o->header.ram_size=rom_bounds[id][2];o->header.bss_size=rom_bounds[id][3];
 return TRUE;
}
BOOL FS_LoadOverlayImage(FSOverlayInfo *o){
 if(!initialized||!o||o->target!=MI_PROCESSOR_ARM9||o->header.id>=COUNT)return FALSE;
 unsigned i=o->header.id;if(state[i])return FALSE;
 /* These SDK-owned module libraries have not yet received native reset sections. */
 if(i==4||i==18||i==60||i==66){PSPNativeMemLog("[OVERLAY] unsupported library lifetime for module %u",i);return FALSE;}
 PSPNativeMemLog("[OVERLAY] load id=%u",i);
 struct Range*r=&ranges[i];memset(r->bss,0,r->bssEnd-r->bss);if(r->dataEnd!=r->data)memcpy(r->data,initialData[i],r->dataEnd-r->data);state[i]=1;return TRUE;
}
void FS_StartOverlay(FSOverlayInfo *o){
 if(!o||o->target!=MI_PROCESSOR_ARM9||o->header.id>=COUNT||state[o->header.id]!=1)abort();
 unsigned i=o->header.id;state[i]=2; /* Constructors can synchronously load another module. */
 for(void (**fn)(void)=(void(**)(void))ranges[i].sinit;fn<(void(**)(void))ranges[i].sinitEnd;fn++)if(*fn)(*fn)();
}
BOOL FS_LoadOverlay(MIProcessor target,FSOverlayID id){FSOverlayInfo o;if(!FS_LoadOverlayInfo(&o,target,id)||!FS_LoadOverlayImage(&o))return FALSE;FS_StartOverlay(&o);return TRUE;}
BOOL FS_UnloadOverlay(MIProcessor target,FSOverlayID id){if(target!=MI_PROCESSOR_ARM9||id>=COUNT||!state[id])return FALSE;state[id]=0;return TRUE;}
BOOL FS_UnloadOverlayImage(FSOverlayInfo *o){return o&&FS_UnloadOverlay(o->target,o->header.id);}
