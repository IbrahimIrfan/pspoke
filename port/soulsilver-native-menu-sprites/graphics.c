#include "menu_state.h"
#include "obj_char_transfer.h"
#include "obj_pltt_transfer.h"
#include "sprite_transfer.h"
#include "unk_0200B150.h"
#include "vram_transfer_manager.h"
#include "gf_gfx_planes.h"
#include "main.h"
#include <string.h>
extern const ObjCharTransferTemplate ssdata_overlay_74_thumb__0223B710;
static enum HeapID Heap(void){return MENU->heap;}
void ov74_02235A74(void *unused){
 void (*callback)(void *);memcpy(&callback,ssdata_overlay_74_thumb_ov74_0223D454+0xea4,sizeof callback);
 if(callback){callback(MENU);callback=NULL;memcpy(ssdata_overlay_74_thumb_ov74_0223D454+0xea4,&callback,sizeof callback);}
 GF_RunVramTransferTasks();OamManager_ApplyAndResetBuffers();if(MENU->bg)DoScheduledBgGpuUpdates(MENU->bg);OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}
void ov74_0223563C(void){
 ObjCharTransferTemplate t=ssdata_overlay_74_thumb__0223B710;t.heapID=Heap();
 ObjCharTransfer_InitEx(&t,16,16);ObjPlttTransfer_Init(30,Heap());ObjCharTransfer_ClearBuffers();ObjPlttTransfer_Reset();
}
void ov74_02235690(void){
 NNS_G2dInitOamManagerModule();OamManager_Create(0,126,0,32,0,126,0,32,Heap());
 MENU->list=G2dRenderer_Init(128,&MENU->renderer,Heap());G2dRenderer_SetSubSurfaceCoords(&MENU->renderer,0,0x100000);MENU->subY=0xc0000;
 for(int i=0;i<6;i++)MENU->managers[i]=Create2DGfxResObjMan(32,i,Heap());
}
void ov74_02235728(NarcId narc,u32 chars,u32 palette,u32 cells,u32 anim,u32 screen){
 struct MenuGraphics*m=MENU;int vram=screen?2:1;BOOL compressed=narc!=18;
 if(screen>1){GF_AssertFail();return;}
 SpriteResource **r=m->resources[screen];
 if(chars!=~0u)r[0]=AddCharResObjFromNarc(m->managers[0],narc,chars,compressed,screen,vram,Heap());
 if(palette!=~0u)r[1]=AddPlttResObjFromNarc(m->managers[1],narc,palette,FALSE,screen,vram,3,Heap());
 if(cells!=~0u)r[2]=AddCellOrAnimResObjFromNarc(m->managers[2],narc,cells,compressed,screen,2,Heap());
 if(anim!=~0u)r[3]=AddCellOrAnimResObjFromNarc(m->managers[3],narc,anim,compressed,screen,3,Heap());
 SpriteTransfer_CreateCharTransferTask(r[0]);SpriteTransfer_CreatePlttTransferTask(r[1]);
 CreateSpriteResourcesHeader(&m->headers[screen],screen,screen,screen,screen,-1,-1,0,0,m->managers[0],m->managers[1],m->managers[2],m->managers[3],NULL,NULL);
 if(screen)GfGfx_EngineBTogglePlanes(16,TRUE);else GfGfx_EngineATogglePlanes(16,TRUE);
 Main_SetVBlankIntrCB(ov74_02235A74,NULL);
}
Sprite *ov74_02235930(u32 screen,Sprite *sprite,u32 x,u32 y,u32 sequence){
 if(!sprite){
  if(screen>1){GF_AssertFail();return NULL;}
  SpriteTemplate t={0};t.spriteList=MENU->list;t.header=&MENU->headers[screen];t.position.x=x<<12;t.position.y=(y<<12)+(screen?(u32)MENU->subY:0);t.scale=(VecFx32){4096,4096,4096};t.drawPriority=10;t.whichScreen=screen?2:1;t.heapID=Heap();sprite=Sprite_CreateAffine(&t);
 }
 Sprite_SetAnimActiveFlag(sprite,TRUE);Sprite_SetPriority(sprite,0);Sprite_SetAnimCtrlSeq(sprite,sequence);Sprite_SetDrawFlag(sprite,TRUE);return sprite;
}
void ov74_022359BC(void){
 if(MENU->sliding){Sprite_Delete(MENU->sliding);MENU->sliding=NULL;}
 for(int i=0;i<2;i++)if(MENU->resources[i][0])SpriteTransfer_DeleteCharTransferTask(MENU->resources[i][0]);
 for(int i=0;i<2;i++)if(MENU->resources[i][1])SpriteTransfer_DeletePlttTransferTask(MENU->resources[i][1]);
 for(int i=0;i<6;i++){Destroy2DGfxResObjMan(MENU->managers[i]);MENU->managers[i]=NULL;}
 SpriteList_Delete(MENU->list);MENU->list=NULL;OamManager_Free();ObjCharTransfer_Destroy();ObjPlttTransfer_Destroy();Main_SetVBlankIntrCB(NULL,NULL);
}
