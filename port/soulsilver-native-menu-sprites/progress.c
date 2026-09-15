#include "menu_state.h"
void ov74_022358C8(u32 suppressReveal) {
 struct MenuGraphics*m=MENU;
 if(m->sliding){
  if(m->delay)m->delay--;
  else {
   if(!Sprite_GetDrawFlag(m->sliding)&&!suppressReveal)Sprite_SetDrawFlag(m->sliding,TRUE);
   VecFx32 *p=Sprite_GetMatrixPtr(m->sliding);
   if(p->y<0x180000)p->y=(u32)p->y+0x3000;
  }
 }
 if(m->list)SpriteList_RenderAndAnimateSprites(m->list);
}
void ov74_022358BC(void){ov74_022358C8(0);}
