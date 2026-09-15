#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <string.h>
#include "render_window.h"
#include "gf_gfx_loader.h"
PSP_MODULE_INFO("SS window proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
static unsigned checks,failures;static int n,copies,clears,arrow;static Window win;static BgConfig config;
struct Rect{unsigned tile,x,y,w,h,pal;}rect[20];
#define CHECK(x) do{checks++;if(!(x)){if(failures<8)printf("FAIL %d\n",__LINE__);failures++;}}while(0)
u8 GetWindowBgId(Window*w){CHECK(w==&win);return w->bgId;}u8 GetWindowX(Window*w){CHECK(w==&win);return w->tilemapLeft;}u8 GetWindowY(Window*w){CHECK(w==&win);return w->tilemapTop;}u8 GetWindowWidth(Window*w){CHECK(w==&win);return w->width;}u8 GetWindowHeight(Window*w){CHECK(w==&win);return w->height;}
void FillBgTilemapRect(BgConfig*b,u8 bg,u16 t,u8 x,u8 y,u8 w,u8 h,u8 pal){CHECK(b==&config&&bg==win.bgId&&n<20);if(n<20)rect[n++]=(struct Rect){t,x,y,w,h,pal};}
void CopyWindowToVram(Window*w){CHECK(w==&win&&n>0&&arrow==-1);copies++;}
void ClearWindowTilemapAndCopyToVram(Window*w){CHECK(w==&win&&n==1);clears++;}
void TextPrinter_SetDownArrowBaseTile(int tile){CHECK(n==17);arrow=tile;}
static int stage,kind,frame,layer,pal,heap,tile;
u32 GfGfxLoader_LoadCharData(NarcId id,s32 member,BgConfig*b,GFBgLayer l,u32 t,u32 bytes,BOOL zipped,enum HeapID h){CHECK(stage++==0);CHECK(id==38&&member==(kind?frame+2:frame?1:0)&&b==&config&&l==layer&&t==tile&&bytes==0&&!zipped&&h==heap);return 0;}
void GfGfxLoader_GXLoadPal(NarcId id,s32 member,enum GFPalLoadLocation loc,enum GFPalSlotOffset off,u32 bytes,enum HeapID h){CHECK(stage++==1);CHECK(id==38&&member==(kind?frame+26:frame==2?46:25)&&loc==(layer<4?0:4)&&off==pal*32&&bytes==32&&h==heap);}
static void reset(void){n=copies=clears=0;arrow=-1;memset(rect,0,sizeof rect);}
static void verifyRect(int i,int t,int x,int y,int w,int h,int p){CHECK(rect[i].tile==(t&65535));CHECK(rect[i].x==(x&255)&&rect[i].y==(y&255));CHECK(rect[i].w==(w&255)&&rect[i].h==(h&255)&&rect[i].pal==(p&255));}
int main(void){scePowerSetClockFrequency(333,333,166);win.bgConfig=&config;
 for(kind=0;kind<2;kind++)for(frame=0;frame<256;frame++)for(layer=0;layer<8;layer++){pal=(frame*13)&255;tile=(frame*313)&65535;heap=frame+1;stage=0;if(kind)LoadUserFrameGfx2(&config,layer,tile,pal,frame,heap);else LoadUserFrameGfx1(&config,layer,tile,pal,frame,heap);CHECK(stage==2);}
 for(int k=0;k<2;k++)for(unsigned v=0;v<65536;v++)for(int skip=0;skip<2;skip++){
  win.tilemapLeft=v>>8;win.tilemapTop=v;win.width=(v*17)>>8;win.height=v*31;win.bgId=v&7;unsigned t=(v*37)&65535,p=v&255;reset();if(k)DrawFrameAndWindow2(&win,skip,t,p);else DrawFrameAndWindow1(&win,skip,t,p);
  CHECK(n==(k?17:8));CHECK(copies==!skip&&clears==0&&arrow==(k?(int)t:-1));
  int colCount=k?6:3;int pos=0;int cols[6],widths[6];
  if(k){cols[0]=win.tilemapLeft-2;cols[1]=win.tilemapLeft-1;cols[2]=win.tilemapLeft;cols[3]=win.tilemapLeft+win.width;cols[4]=cols[3]+1;cols[5]=cols[3]+2;}
  else{cols[0]=win.tilemapLeft-1;cols[1]=win.tilemapLeft;cols[2]=win.tilemapLeft+win.width;}
  for(int row=0;row<3;row++)for(int col=0;col<colCount;col++){int center=k?2:1;if(row==1&&col==center)continue;int y=row==0?win.tilemapTop-1:row==1?win.tilemapTop:win.tilemapTop+win.height;verifyRect(pos++,t+row*colCount+col,cols[col],y,col==center?win.width:1,row==1?win.height:1,p);}
  reset();if(k)ClearFrameAndWindow2(&win,skip);else sub_0200E5D4(&win,skip);CHECK(n==1&&clears==!skip&&copies==0&&arrow==-1);verifyRect(0,0,win.tilemapLeft-(k?2:1),win.tilemapTop-1,win.width+(k?5:2),win.height+2,0);
 }
 printf("[SS-WINDOW] checks=%u failures=%u\n",checks,failures);sceKernelExitGame();return failures!=0;}
