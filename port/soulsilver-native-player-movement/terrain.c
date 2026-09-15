#include "player_avatar.h"
#include "map_object.h"
#include "metatile_behavior.h"
BOOL sub_02060EEC(LocalMapObject*object,u32 behavior){return MapObject_CheckFlag28(object)==1&&sub_0205BA30((u8)behavior)==1;}
BOOL sub_0205E078(PlayerAvatar*avatar,u32 behavior,int direction){
 if(direction!=-1||PlayerAvatar_GetState(avatar)!=1||!PlayerAvatar_CheckBikeStateLocked(avatar))return FALSE;
 return sub_02060EEC(PlayerAvatar_GetMapObject(avatar),behavior)==1;
}
int sub_0205D01C(PlayerAvatar*avatar,int direction){
 u32 behavior=sub_0205F504(PlayerAvatar_GetMapObject(avatar));
 if(sub_0205E078(avatar,behavior,direction)==1)return 2;
 if(PlayerAvatar_CheckFlag1(avatar)==1)return 0;
 // The original two-entry table is {MetatileBehavior_IsIce,1},{NULL,3}.
 return MetatileBehavior_IsIce((u8)behavior)==1?1:0;
}
