#include <nitro.h>
#include <nnsys.h>
#pragma pack(push,4)
#include "map_object.h"
#include "unk_02062108.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
_Static_assert(offsetof(LocalMapObject,movementCmd)==0xa4,"SS command offset");
_Static_assert(offsetof(LocalMapObject,movementStep)==0xa8,"SS step offset");
BOOL MapObject_AreBitsSetForMovementScriptInit(LocalMapObject *o){
 if(!MapObject_TestFlagsBits(o,1))return FALSE;
 if(MapObject_TestFlagsBits(o,2)==TRUE)return FALSE;
 if(MapObject_TestFlagsBits(o,0x10)==TRUE&&!MapObject_TestFlagsBits(o,0x20))return FALSE;
 return TRUE;
}
void MapObject_SetHeldMovement(LocalMapObject *o,u32 command){
 /* Original Thumb cmp/blt is signed; preserve high-bit command behavior. */
 if((s32)command>=0x71)GF_AssertFail();
 MapObject_SetMovementCommand(o,command);MapObject_SetMovementStep(o,0);
 MapObject_SetFlagsBits(o,0x10);MapObject_ClearFlagsBits(o,0x20);
}
void MapObject_ForceSetHeldMovement(LocalMapObject *o,u32 command){
 MapObject_SetMovementCommand(o,command);MapObject_SetMovementStep(o,0);MapObject_ClearFlagsBits(o,0x20);
}
BOOL MapObject_IsMovementPaused(LocalMapObject *o){return !MapObject_TestFlagsBits(o,0x10)||MapObject_TestFlagsBits(o,0x20);}
BOOL MapObject_ClearHeldMovementIfActive(LocalMapObject *o){
 if(!MapObject_TestFlagsBits(o,0x10))return TRUE;
 if(!MapObject_TestFlagsBits(o,0x20))return FALSE;
 MapObject_ClearFlagsBits(o,0x30);return TRUE;
}
void MapObject_ClearHeldMovement(LocalMapObject *o){
 MapObject_ClearFlagsBits(o,0x10);MapObject_SetFlagsBits(o,0x20);
 MapObject_SetMovementCommand(o,0xff);MapObject_SetMovementStep(o,0);
}
/* Native object movement dispatcher. Unported effect/script callbacks below
 * remain real unresolved fatal boundaries in the diagnostic, never no-ops. */
extern void sub_02061070(LocalMapObject *),sub_0205FEDC(LocalMapObject *),sub_02062400(LocalMapObject *);
extern void sub_02060020(LocalMapObject *),sub_0205FF6C(LocalMapObject *),sub_02060114(LocalMapObject *),sub_0206008C(LocalMapObject *);
extern BOOL sub_02061108(LocalMapObject *),sub_02063A1C(LocalMapObject *);
extern void sub_0205F430(LocalMapObject *);
BOOL sub_0205FD98(LocalMapObject *o){
 if(MapObject_CheckSingleMovement(o)==TRUE)return TRUE;
 if(!MapObject_GetFlagsBitsMask(o,0x1800))return TRUE;
 if(MapObject_GetMovement(o)==0x32||MapObject_GetMovement(o)==0x30)return TRUE;
 u32 flags=MapObject_GetFlags(o);
 if((flags&0x1000)&&!(flags&0x800000))return FALSE;
 if((flags&0x800)&&!sub_0205F8D0(o))return FALSE;
 return TRUE;
}
void sub_0205FE0C(LocalMapObject *o){if(MapObject_GetFlagsBitsMask(o,0x1000))sub_02061070(o);}
void sub_0205FE24(LocalMapObject *o){if(MapObject_GetFlagsBitsMask(o,0x800)&&sub_02061108(o)==TRUE)MapObject_SetStartMovement(o);}
void sub_0205FE48(LocalMapObject *o){if(MapObject_GetFlagsBitsMask(o,4))sub_0205FEDC(o);MapObject_ClearFlagsBits(o,0x10004);}
void sub_0205FE6C(LocalMapObject *o){if(MapObject_GetFlagsBitsMask(o,0x10000))sub_02060020(o);else if(MapObject_GetFlagsBitsMask(o,4))sub_0205FF6C(o);MapObject_ClearFlagsBits(o,0x10004);}
void sub_0205FEA4(LocalMapObject *o){if(MapObject_GetFlagsBitsMask(o,0x20000))sub_02060114(o);else if(MapObject_GetFlagsBitsMask(o,8))sub_0206008C(o);MapObject_ClearFlagsBits(o,0x20008);}
void sub_0205FD30(LocalMapObject *o){
 if(sub_0205F5E8(o,2))return;
 sub_0205FE0C(o);sub_0205FE24(o);sub_0205FE48(o);
 if(MapObject_GetFlagsBitsMask(o,0x10))sub_02062400(o);
 else if(!MapObject_CheckMovementPaused(o)&&sub_0205FD98(o)==TRUE&&!sub_02063A1C(o))sub_0205F430(o);
 sub_0205FE6C(o);sub_0205FEA4(o);
}
/* Real object-type callback tables from SS assembly. Default types have original
 * empty initialization / false predicates; special types retain real callbacks. */
extern void (*const ssdata_unk_020632B0__020FE104[])(LocalMapObject *);
extern BOOL (*const ssdata_unk_020632B0__020FE134[])(LocalMapObject *);
extern BOOL (*const ssdata_unk_020632B0__020FE164[])(LocalMapObject *);
void sub_02063AFC(LocalMapObject *o){(void)o;} /* original bx lr */
BOOL sub_02063B00(LocalMapObject *o){(void)o;return FALSE;} /* original mov r0,0 */
BOOL sub_02063B04(LocalMapObject *o){(void)o;return FALSE;}
static u32 ObjectType(LocalMapObject *o){u32 type=MapObject_GetType(o);if(type>=12)GF_AssertFail();return type;}
void sub_02063A40(LocalMapObject *o){ssdata_unk_020632B0__020FE104[ObjectType(o)](o);}
BOOL sub_02063A5C(LocalMapObject *o){return ssdata_unk_020632B0__020FE134[ObjectType(o)](o);}
BOOL sub_02063A78(LocalMapObject *o){return ssdata_unk_020632B0__020FE164[ObjectType(o)](o);}
BOOL sub_02063A1C(LocalMapObject *o){if(!sub_02063A5C(o))return FALSE;return sub_02063A78(o)!=0;}
/* Independent byte-level expected state for all flag low-byte combinations,
 * randomized upper flags/whole objects, command values and surrounding guards.
 * Calls the real existing game accessors; no accessor replacement in production. */
void SSNativeMapMovementProof(void){
 unsigned checks=0,errors=0;u32 seed=0x519fabcd;
 struct {u32 pre[4];LocalMapObject o;u32 post[4];} base,got,want;
 for(u32 k=0;k<4096;k++){
  for(unsigned j=0;j<sizeof(base);j++){seed=seed*1664525u+1013904223u;((u8*)&base)[j]=seed>>24;}
  base.o.flags=(base.o.flags&~255u)|(k&255u);u32 f=base.o.flags;
  for(unsigned op=0;op<8;op++){
   got=want=base;BOOL actual=0,expected=0;u32 command=k&1?(0x80000000u|seed):k%113;
   switch(op){
   case 0:actual=MapObject_AreBitsSetForMovementScriptInit(&got.o);expected=(f&1)&&!(f&2)&&(!(f&16)||(f&32));break;
   case 1:actual=MapObject_IsMovementPaused(&got.o);expected=!(f&16)||(f&32);break;
   case 2:actual=MapObject_ClearHeldMovementIfActive(&got.o);expected=!(f&16)||(f&32);if((f&48)==48)want.o.flags=f&~48u;break;
   case 3:MapObject_ClearHeldMovement(&got.o);want.o.flags=(f&~16u)|32;want.o.movementCmd=255;want.o.movementStep=0;break;
   case 4:MapObject_SetHeldMovement(&got.o,command);want.o.flags=(f|16)&~32u;want.o.movementCmd=command;want.o.movementStep=0;break;
   case 7:{static const u32 types[]={0,1,2,3,7,8,9,10,11};got.o.type=want.o.type=types[k%9];sub_02063A40(&got.o);actual=sub_02063A1C(&got.o)|sub_02063A78(&got.o);expected=0;break;}
   case 6:actual=sub_0205FD98(&got.o);expected=(f&2)||!(f&0x1800)||base.o.movement==0x32||base.o.movement==0x30||(!((f&0x1000)&&!(f&0x800000))&&!((f&0x800)&&!(base.o.flags2&4)));break;
   case 5:MapObject_ForceSetHeldMovement(&got.o,seed);want.o.flags=f&~32u;want.o.movementCmd=seed;want.o.movementStep=0;break;
   }
   checks++;if(!!actual!=!!expected)errors++;
   for(unsigned j=0;j<sizeof(got);j++){checks++;if(((u8*)&got)[j]!=((u8*)&want)[j])errors++;}
  }
 }
 printf("[SS-MAP-MOVEMENT] checks=%u errors=%u\n",checks,errors);if(errors)abort();
}
