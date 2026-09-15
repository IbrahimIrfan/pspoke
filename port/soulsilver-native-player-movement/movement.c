#include "player_avatar.h"
#include "map_object.h"
#include "unk_02062108.h"
#include "unk_0205CB48.h"
extern int sub_0205D01C(PlayerAvatar *,int);
BOOL sub_0205DE64(u32 command){return command>=0x1c&&command<=0x1f;}
void PlayerAvatar_UpdateMovement(PlayerAvatar *avatar){
 u32 move=PlayerAvatar_GetMoveState(avatar),previous=PlayerAvatar_GetPlayerMoveState(avatar);
 LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);
 PlayerAvatar_SetPlayerMoveState(avatar,0);
 int terrain=sub_0205D01C(avatar,-1);
 if(terrain!=0&&terrain!=2){PlayerAvatar_SetPlayerMoveState(avatar,2);return;}
 if(!MapObject_AreBitsSetForMovementScriptInit(object)){
  if(move==1){
   if(sub_0205DE64(MapObject_GetMovementCommand(object))==1)return;
   PlayerAvatar_SetPlayerMoveState(avatar,(previous==0||previous==3)?1:2);
  }else if(move==2)PlayerAvatar_SetPlayerMoveState(avatar,2);
 }else if(MapObject_IsMovementPaused(object)==1){
  if((move==1||move==2)&&previous!=0)PlayerAvatar_SetPlayerMoveState(avatar,previous==3?0:3);
 }
}
void sub_0205CF44(PlayerAvatar *avatar){PlayerAvatar_SetMoveState(avatar,0);PlayerAvatar_SetPlayerMoveState(avatar,0);PlayerAvatar_ClearUnk24ClearFlag2(avatar);}
BOOL sub_0205CF60(PlayerAvatar *avatar){
 u32 move=PlayerAvatar_GetMoveState(avatar),state=PlayerAvatar_GetPlayerMoveState(avatar);
 if(move==0||move==2)return TRUE;
 if(move!=1)return FALSE;
 if(state==0||state==3)return TRUE;
 LocalMapObject *object=PlayerAvatar_GetMapObject(avatar);
 if(MapObject_AreBitsSetForMovementScriptInit(object)==1)return TRUE;
 return sub_0205DE64(MapObject_GetMovementCommand(object))==1;
}
