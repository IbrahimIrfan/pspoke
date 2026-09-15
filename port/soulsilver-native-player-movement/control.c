#include "player_avatar.h"
#include "unk_0205CB48.h"
extern int sub_0205DDD4(PlayerAvatar*,u16,u16);
extern BOOL sub_0205CBEC(PlayerAvatar*,int);
extern void sub_0205CC4C(PlayerAvatar*,int,u16,u16);
extern void Field_PlayerAvatar_ApplyTransitionFlags(PlayerAvatar*);
extern BOOL sub_0205D004(PlayerAvatar*,int);
extern BOOL sub_0205D40C(PlayerAvatar*,int);
extern void ov01_021F2F24(PlayerAvatar*);
extern void ov01_021F2EDC(PlayerAvatar*);
extern void sub_0205D340(PlayerAvatar*,MapLoadManager*,int,u16,u16);
extern void sub_0205CC74(PlayerAvatar*);
extern void sub_0205CC94(PlayerAvatar*);
void PlayerAvatar_MoveControl(PlayerAvatar *avatar,MapLoadManager *manager,int direction,u16 newKeys,u16 heldKeys,int allowStep){
 if(direction==-1)direction=sub_0205DDD4(avatar,newKeys,heldKeys);
 if(!sub_0205CBEC(avatar,direction))return;
 sub_0205CC4C(avatar,direction,newKeys,heldKeys);
 Field_PlayerAvatar_ApplyTransitionFlags(avatar);
 if(sub_0205D004(avatar,direction)==1){ov01_021F2F24(avatar);return;}
 if(PlayerAvatar_GetState(avatar)==0){
  if(sub_0205D40C(avatar,direction))ov01_021F2F24(avatar);
  else if(allowStep==1)ov01_021F2EDC(avatar);
 }
 sub_0205D340(avatar,manager,direction,newKeys,heldKeys);
 sub_0205CC74(avatar);
 sub_0205CC94(avatar);
}
