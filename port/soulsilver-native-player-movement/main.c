#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include "player_avatar.h"
#include "map_object.h"
#include "unk_02062108.h"
#include "unk_0205CB48.h"
PSP_MODULE_INFO("SS player movement proof",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
static unsigned checks,failures;static int terrain,scriptInit,paused;static LocalMapObject object;static PlayerAvatar *current;
#define CHECK(x) do{checks++;if(!(x)){if(failures<8)printf("FAIL %d\n",__LINE__);failures++;}}while(0)
int sub_0205D01C(PlayerAvatar *a,int direction){CHECK(a==current&&direction==-1&&a->playerMoveState==0);return terrain;}
BOOL MapObject_AreBitsSetForMovementScriptInit(LocalMapObject *p){CHECK(p==&object);return scriptInit;}
BOOL MapObject_IsMovementPaused(LocalMapObject*p){CHECK(p==&object);return paused;}
BOOL sub_0205DE64(u32);
int main(void){
 const unsigned values[]={0,1,2,3,4,0x7fffffff,0x80000000,0xffffffff};
 const unsigned cmds[]={0,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0xff,0xffffffff};
 const unsigned activeTable[3][4]={{0,0,0,0},{1,2,2,1},{2,2,2,2}};
 const unsigned pausedTable[3][4]={{0,0,0,0},{0,3,3,0},{0,3,3,0}};
 for(unsigned m=0;m<8;m++)for(unsigned p=0;p<8;p++)for(unsigned t=0;t<8;t++)for(int i=-1;i<3;i++)for(int z=-1;z<3;z++)for(unsigned cmd=0;cmd<9;cmd++){
  PlayerAvatar avatar;memset(&avatar,0xa5,sizeof avatar);avatar.moveState=values[m];avatar.playerMoveState=values[p];avatar.mapObject=&object;current=&avatar;object.movementCmd=cmds[cmd];terrain=values[t];scriptInit=i;paused=z;PlayerAvatar expected=avatar;
  unsigned previous=values[p],v=values[m],x=0;unsigned tablePrevious=previous<4?previous:1;
  if(terrain!=0&&terrain!=2)x=2;
  else if(!i){if(v<3)x=activeTable[v][tablePrevious];if(v==1&&cmds[cmd]>=0x1c&&cmds[cmd]<=0x1f)x=0;}
  else if(z==1&&v<3)x=pausedTable[v][tablePrevious];
  expected.playerMoveState=x;PlayerAvatar_UpdateMovement(&avatar);CHECK(!memcmp(&avatar,&expected,sizeof avatar));
  avatar.playerMoveState=previous;expected=avatar;BOOL ready=v==0||v==2||(v==1&&(previous==0||previous==3||i==1||(cmds[cmd]>=0x1c&&cmds[cmd]<=0x1f)));CHECK(sub_0205CF60(&avatar)==ready);CHECK(!memcmp(&avatar,&expected,sizeof avatar));
 }
 for(unsigned v=0;v<65536;v++){CHECK(sub_0205DE64(v)==(v==28||v==29||v==30||v==31));CHECK(sub_0205DE64(v|0xffff0000)==0);PlayerAvatar a;memset(&a,v&255,sizeof a);a.flags=(v<<16)|(v^0x5a5a);a.unk24=v;a.moveState=v;a.playerMoveState=~v;PlayerAvatar e=a;e.flags&=~4;e.unk24=e.moveState=e.playerMoveState=0;sub_0205CF44(&a);CHECK(!memcmp(&a,&e,sizeof a));}
 printf("[SS-PLAYER-MOVEMENT] checks=%u failures=%u\n",checks,failures);sceKernelExitGame();return failures!=0;}
