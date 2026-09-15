#include "billboard_gfx_sequence.h"
// The original SS API returns packed bytes in a scalar register. Keep that ABI
// separate from the public typed helper, whose native structure return may differ.
u32 sub_02026DE0(const BillboardGfxSequence *sequence,u32 frame){
 u32 i=0;
 while(i<sequence->seqCount-1&&sequence->startFrame[i+1]<=frame)i++;
 return sequence->textureIdx[i]|((u32)sequence->plttIdx[i]<<8);
}
BillboardTexPlttIndex BillboardGfxSequence_GetTexPlttIndexAt(const BillboardGfxSequence *sequence,u16 frame){
 u32 packed=sub_02026DE0(sequence,frame);
 BillboardTexPlttIndex result={(u8)packed,(u8)(packed>>8)};return result;
}
void sub_02026E18(const void*data,BillboardGfxSequence*sequence){
 const u8*bytes=data;sequence->seqCount=*(const u32*)bytes;
 sequence->startFrame=(const u16*)(bytes+4);
 sequence->textureIdx=bytes+4+sequence->seqCount*2;
 sequence->plttIdx=sequence->textureIdx+sequence->seqCount;
}
void BillboardGfxSequence_SetData(const void*data,BillboardGfxSequence*sequence){sub_02026E18(data,sequence);}
