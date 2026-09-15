#include <stdint.h>
#include "field/map_prop.h"
/* Public source PR502 renamed these functions; this older C unit retained
   their original labels and 32-bit pointer-shaped integer declarations. */
int ov01_021F3B44(int manager,u8 index){return (int)(uintptr_t)MapPropManager_GetMapPropByIndex((MapPropManager *)(uintptr_t)(u32)manager,index);}
int ov01_021F3B34(int prop){return MapProp_GetBuildModel((MapProp *)(uintptr_t)(u32)prop);}
void ov01_021F3B0C(VecFx32 *out,void *prop){MapProp_GetTranslation(out,(MapProp *)prop);}
