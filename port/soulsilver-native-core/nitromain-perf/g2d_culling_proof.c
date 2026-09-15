#include <nitro.h>
#include <nnsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "g2d_culling_vectors.h"
extern BOOL sub_02025C98(const NNSG2dCellData*,const MtxFx32*,const NNSG2dViewRect*);
void SSNativeCullingProof(void){unsigned errors=0,checks=0;for(unsigned k=0;k<sizeof(cullingVectors)/sizeof(*cullingVectors);k++){
 const struct CullingVector *v=&cullingVectors[k];NNSG2dCellDataWithBR c={0};MtxFx32 m;NNSG2dViewRect rect;c.cellData.cellAttr=v->attr;memcpy(&c.boundingRect,v->bounds,8);memcpy(&m,v->matrix,24);memcpy(&rect,v->rect,16);NNSG2dCellDataWithBR old=c;
 unsigned got=sub_02025C98(&c.cellData,&m,&rect);checks++;if(got!=v->expected){if(errors<3)printf("[SS-CULL] vector%u got%u expected%u\n",k,got,v->expected);errors++;}checks++;if(memcmp(&c,&old,sizeof(c))||memcmp(&m,v->matrix,24)||memcmp(&rect,v->rect,16))errors++;
 }printf("[SS-CULL] checks=%u errors=%u\n",checks,errors);if(errors)abort();}
