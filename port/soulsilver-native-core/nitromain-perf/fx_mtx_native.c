/* C ports of the NitroSDK fx matrix routines that the SDK port left as
 * SIM_assert_always_msg("Not implemented") stubs (hand assembly on the DS).
 * SoulSilver's starter-selection app calls MTX_Scale43_ every frame and stalled
 * in the stub; in Platinum the particle library's polygon draws call it, so any
 * battle animation with polygon particles (Sing's notes, for one) aborted.
 * Both games link this file. Ported line for line from
 * the assembly bodies in libntr/libraries/fx/src/fx_mtx{33,43,44}.c. The
 * archive copies of these symbols are weakened so these definitions win. */
#include <nitro.h>
void MTX_Copy33To43_(const MtxFx33 *s, MtxFx43 *d){
 d->_00=s->_00;d->_01=s->_01;d->_02=s->_02; d->_10=s->_10;d->_11=s->_11;d->_12=s->_12; d->_20=s->_20;d->_21=s->_21;d->_22=s->_22;
 d->_30=0;d->_31=0;d->_32=0;}
void MTX_Copy33To44_(const MtxFx33 *s, MtxFx44 *d){
 d->_00=s->_00;d->_01=s->_01;d->_02=s->_02;d->_03=0; d->_10=s->_10;d->_11=s->_11;d->_12=s->_12;d->_13=0;
 d->_20=s->_20;d->_21=s->_21;d->_22=s->_22;d->_23=0; d->_30=0;d->_31=0;d->_32=0;d->_33=FX32_ONE;}
void MTX_Transpose33_(const MtxFx33 *s, MtxFx33 *d){
 MtxFx33 t=*s; d->_00=t._00;d->_01=t._10;d->_02=t._20; d->_10=t._01;d->_11=t._11;d->_12=t._21; d->_20=t._02;d->_21=t._12;d->_22=t._22;}
void MTX_Scale33_(MtxFx33 *d, fx32 x, fx32 y, fx32 z){
 d->_00=x;d->_01=0;d->_02=0; d->_10=0;d->_11=y;d->_12=0; d->_20=0;d->_21=0;d->_22=z;}
void MTX_Copy43To44_(const MtxFx43 *s, MtxFx44 *d){
 d->_00=s->_00;d->_01=s->_01;d->_02=s->_02;d->_03=0; d->_10=s->_10;d->_11=s->_11;d->_12=s->_12;d->_13=0;
 d->_20=s->_20;d->_21=s->_21;d->_22=s->_22;d->_23=0; d->_30=s->_30;d->_31=s->_31;d->_32=s->_32;d->_33=FX32_ONE;}
void MTX_Transpose43_(const MtxFx43 *s, MtxFx43 *d){
 MtxFx43 t=*s; d->_00=t._00;d->_01=t._10;d->_02=t._20; d->_10=t._01;d->_11=t._11;d->_12=t._21; d->_20=t._02;d->_21=t._12;d->_22=t._22;
 d->_30=0;d->_31=0;d->_32=0;}
void MTX_Scale43_(MtxFx43 *d, fx32 x, fx32 y, fx32 z){
 d->_00=x;d->_01=0;d->_02=0; d->_10=0;d->_11=y;d->_12=0; d->_20=0;d->_21=0;d->_22=z; d->_30=0;d->_31=0;d->_32=0;}
void MTX_Transpose44_(const MtxFx44 *s, MtxFx44 *d){
 MtxFx44 t=*s; d->_00=t._00;d->_01=t._10;d->_02=t._20;d->_03=t._30; d->_10=t._01;d->_11=t._11;d->_12=t._21;d->_13=t._31;
 d->_20=t._02;d->_21=t._12;d->_22=t._22;d->_23=t._32; d->_30=t._03;d->_31=t._13;d->_32=t._23;d->_33=t._33;}
void MTX_Scale44_(MtxFx44 *d, fx32 x, fx32 y, fx32 z){
 d->_00=x;d->_01=0;d->_02=0;d->_03=0; d->_10=0;d->_11=y;d->_12=0;d->_13=0; d->_20=0;d->_21=0;d->_22=z;d->_23=0; d->_30=0;d->_31=0;d->_32=0;d->_33=FX32_ONE;}
