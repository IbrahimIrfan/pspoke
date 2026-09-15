#include <nitro.h>
#include <nnsys.h>
#include <string.h>
#include <stddef.h>
#pragma pack(push,4)
#include "bg_window.h"
#include "screen_fade.h"
#include "overlay_manager.h"
extern u32 ssdata_overlay_74_thumb_ov74_0223D450;
extern u8 ssdata_overlay_74_thumb_ov74_0223D454[];
extern const BgTemplate ssdata_overlay_74_thumb__0223B720;
static u32 *MenuWords(void){return (u32 *)ssdata_overlay_74_thumb_ov74_0223D454;}
/* All mutable state belongs to the original overlay74 reset range. */
void ov74_0223512C(enum HeapID heap){ssdata_overlay_74_thumb_ov74_0223D450=heap;}
void *ov74_02235138(u32 size){return Heap_Alloc(ssdata_overlay_74_thumb_ov74_0223D450,size);}
void ov74_022352A0(enum HeapID heap){memset(MenuWords(),0,0xea8);MenuWords()[1]=1;MenuWords()[2]=heap;}
void *ov74_022352D0(OverlayManager *manager,enum HeapID heap,u32 size,u32 heapSize){Heap_Create(HEAP_ID_3,heap,heapSize);void *data=OverlayManager_CreateAndGetData(manager,size,heap);memset(data,0,size);sub_0200FBF4(0,0);sub_0200FBF4(1,0);return data;}
void ov74_02235308(BgConfig *bg,u8 layer,u32 size,u32 screenBase,u32 charBase){BgTemplate t=ssdata_overlay_74_thumb__0223B720;t.size=size;if(size==1)t.bufferSize=2048;else if(size==2||size==3)t.bufferSize=4096;else if(size==4)t.bufferSize=8192;t.screenBase=screenBase>>11;t.charBase=charBase>>14;InitBgFromTemplate(bg,layer,&t,0);BgClearTilemapBufferAndCommit(bg,layer);}
void ov74_02235390(u32 mode){MenuWords()[4]=mode;}
void ov74_0223539C(u32 fade,u32 next,int *state,u32 wait){BeginNormalPaletteFade(0,fade,fade,MenuWords()[4]?0x7fff:0,6,1,MenuWords()[2]);if(state)*state=wait;MenuWords()[3]=next;}
void ov74_022353FC(int *state){if(IsPaletteFadeFinished())*state=MenuWords()[3];}
typedef struct MenuText {u32 style,drawFrame,erase,unknown;Window *window;void *format;u32 width,height,x,y,tile,layer,palette1,archive,frameTile,palette2,font,color;u8 fill,pad[3];u32 lastMessage,speed;} MenuText;
_Static_assert(sizeof(MenuText)==0x54,"SS menu text record");
_Static_assert(offsetof(MenuText,window)==0x10,"SS menu window pointer");
_Static_assert(offsetof(MenuText,color)==0x44,"SS menu text colors");
void ov74_02235414(MenuText *r,Window *w,u32 pal1,u32 archive,u32 frameTile,u32 pal2){memset(r,0,0x54);r->drawFrame=1;r->erase=1;r->lastMessage=0xffffffff;r->window=w;r->palette1=pal1;r->archive=archive;r->frameTile=frameTile;r->palette2=pal2;r->color=0x1020f;r->fill=15;r->speed=255;}
void ov74_02235464(MenuText *r,u32 width,u32 height,u32 tile){r->width=width;r->height=height;r->tile=tile;}
void ov74_0223546C(MenuText *r,u32 style,u32 font){r->style=style;r->font=font;}
void ov74_02235474(MenuText *r,u32 x,u32 y){r->x=x;r->y=y;}
/* Bounded diagnostic checks, discarded by --gc-sections when no probe calls it. */
#include <stdio.h>
#include <stdlib.h>
extern void *ssdata_overlay_74_thumb_sPmAgbCartridgeSpec;
extern u8 ssdata_overlay_74_thumb_ov74_0223D33C[];
u32 PmAgbCartridgeGetOffsets(u32 arg);
u32 ov74_02235230(void);
void SSNativeMenuProof(void){
 u8 saved[0xea8];memcpy(saved,MenuWords(),sizeof(saved));u32 oldHeap=ssdata_overlay_74_thumb_ov74_0223D450;unsigned checks=0,errors=0;
 for(u32 k=0;k<256;k++){
  u32 heap=k*0x9e3779b9u;memset(MenuWords(),0xa5,sizeof(saved));ov74_022352A0((enum HeapID)heap);
  for(unsigned j=0;j<sizeof(saved)/4;j++){u32 wanted=j==1?1:j==2?heap:0;checks++;if(MenuWords()[j]!=wanted)errors++;}
  ov74_0223512C((enum HeapID)heap);checks++;if(ssdata_overlay_74_thumb_ov74_0223D450!=heap)errors++;
  struct {u32 before;MenuText r;u32 after;} item,expect;memset(&item,0xa5,sizeof(item));expect=item;memset(&expect.r,0,sizeof(expect.r));
  u32 *words=(u32*)&expect.r;words[1]=1;words[2]=1;words[4]=0x12345678;words[12]=heap;words[13]=~heap;words[14]=heap^0x1234;words[15]=heap+4;words[17]=0x1020f;words[18]=15;words[19]=0xffffffff;words[20]=255;
  ov74_02235414(&item.r,(Window*)0x12345678,heap,~heap,heap^0x1234,heap+4);checks++;if(memcmp(&item,&expect,sizeof(item)))errors++;
  ov74_02235464(&item.r,heap,~heap,heap+3);words[6]=heap;words[7]=~heap;words[10]=heap+3;
  ov74_0223546C(&item.r,heap+1,heap+2);words[0]=heap+1;words[16]=heap+2;
  ov74_02235474(&item.r,heap-1,heap-2);words[8]=heap-1;words[9]=heap-2;checks++;if(memcmp(&item,&expect,sizeof(item)))errors++;
 }
 memcpy(MenuWords(),saved,sizeof(saved));ssdata_overlay_74_thumb_ov74_0223D450=oldHeap;
 void *savedSpec=ssdata_overlay_74_thumb_sPmAgbCartridgeSpec;
 u32 savedArg=*(u32 *)(ssdata_overlay_74_thumb_ov74_0223D33C+8);
 for(u32 k=0;k<256;k++){
  u32 arg=k*0x9e3779b9u;ssdata_overlay_74_thumb_sPmAgbCartridgeSpec=NULL;
  checks++;if(PmAgbCartridgeGetOffsets(arg)!=1||*(u32 *)(ssdata_overlay_74_thumb_ov74_0223D33C+8)!=arg||ssdata_overlay_74_thumb_sPmAgbCartridgeSpec)errors++;
  ssdata_overlay_74_thumb_sPmAgbCartridgeSpec=(void*)0x12345678;
  checks++;if(PmAgbCartridgeGetOffsets(~arg)!=12||*(u32 *)(ssdata_overlay_74_thumb_ov74_0223D33C+8)!=~arg||ssdata_overlay_74_thumb_sPmAgbCartridgeSpec!=(void*)0x12345678)errors++;
 }
 ssdata_overlay_74_thumb_sPmAgbCartridgeSpec=savedSpec;*(u32 *)(ssdata_overlay_74_thumb_ov74_0223D33C+8)=savedArg;
 checks++;if(ov74_02235230()!=0)errors++;
 printf("[SS-MENU] record/init checks=%u errors=%u\n",checks,errors);if(errors)abort();
}
#include "msgdata.h"
#include "message_format.h"
#include "font.h"
#include "text.h"
#include "render_window.h"
/* Original fast paths leave r7 unspecified; native C callers ignore the result.
 * Report the SDK's invalid printer ID when no new print operation is started. */
u32 ov74_0223547C(MenuText *r,u32 message){
 u32 printer=0xff;
 if(message!=0xffffffff&&message!=r->lastMessage){
  r->lastMessage=message;if(r->erase==1)FillWindowPixelBuffer(r->window,r->fill);
  MsgData *messages=NewMsgDataFromNarc(1,27,r->archive,MenuWords()[2]);
  MessageFormat *format=r->format;if(!format)format=MessageFormat_New(MenuWords()[2]);
  String *text=ReadMsgData_ExpandPlaceholders(format,messages,r->lastMessage,MenuWords()[2]);
  u32 x=r->x;if(r->unknown){u32 spacing=GetFontAttribute((u8)r->font,2);u32 width=FontID_String_GetWidth(r->font,text,spacing);x=(u32)GetWindowWidth(r->window)*8-width;}
  printer=AddTextPrinterParameterizedWithColor(r->window,r->font,text,x,r->y,r->speed,r->color,NULL);
  if(r->unknown)r->unknown=0;String_Delete(text);if(!r->format)MessageFormat_Delete(format);DestroyMsgData(messages);
 }
 r->speed=255;return printer;
}
u32 ov74_02235568(BgConfig *bg,MenuText *r,u32 x,u32 y,u32 message){
 if(!r->window->bgConfig)AddWindowParameterized(bg,r->window,(u8)r->layer,(u8)x,(u8)y,(u8)r->width,(u8)r->height,(u8)r->palette1,(u16)r->tile);
 else{if(x!=0xffffffff)SetWindowX(r->window,(u8)x);if(y!=0xffffffff)SetWindowY(r->window,(u8)y);}
 u32 printer=ov74_0223547C(r,message);
 if(r->drawFrame==1){if(r->style==0)DrawFrameAndWindow1(r->window,0,(u16)r->frameTile,(u8)r->palette2);else if(r->style==1)DrawFrameAndWindow2(r->window,0,(u16)r->frameTile,(u8)r->palette2);else CopyWindowToVram(r->window);}
 return printer;
}
u32 ov74_02235634(MenuText *r){return r->width*r->height;}

/* PSP has no Slot-2. Preserve the original lookup prelude and its no-cartridge
 * result. A future cartridge bridge must port the active identification path. */
extern void *ssdata_overlay_74_thumb_sPmAgbCartridgeSpec;
extern u8 ssdata_overlay_74_thumb_ov74_0223D33C[];
u32 PmAgbCartridgeGetOffsets(u32 arg){
 *(u32 *)(ssdata_overlay_74_thumb_ov74_0223D33C+8)=arg;
 if(ssdata_overlay_74_thumb_sPmAgbCartridgeSpec)return 12;
 ssdata_overlay_74_thumb_sPmAgbCartridgeSpec=NULL;
 if(!CTRDG_IsAgbCartridge())return 1;
 printf("[SS-UNSUPPORTED] native Slot-2 active cartridge identification\n");abort();
}

u32 ov74_02235230(void){
 CTRDG_Init();
 if(!CTRDG_IsAgbCartridge())return 0;
 printf("[SS-UNSUPPORTED] native Slot-2 accessory identification\n");abort();
}
void ov74_02236034(BOOL enable){
 if(enable==1){printf("[SS-UNSUPPORTED] native Slot-2 cartridge IRQ\n");abort();}
 OS_DisableIrqMask(0x2000);
}
