#include "sound.h"
#include "constants/sndseq.h"
extern const u16 ssdata_unk_02004A44__020F5730[136][2];
extern void GF_SetCurrentPlayingBGM(u16);
extern int sub_0203993C(void);
extern int sub_02034044(int);
extern int ov00_021E7080(void);
void sub_020059E0(int value) { *(u8 *)GF_SdatGetAttrPtr(0x13) = value; }
void sub_02004AB8(u16 seq) { *(u16 *)GF_SdatGetAttrPtr(0x3A) = seq; }
u16 GBSounds_GetDSSeqNoByGBSeqNo(u16 seq) {
    for (unsigned i=0;i<136;i++)
        if (seq==ssdata_unk_02004A44__020F5730[i][1]) return ssdata_unk_02004A44__020F5730[i][0];
    return seq;
}
void sub_02004A60(u16 seq) {
    u16 *current=GF_SdatGetAttrPtr(0xA);
    if (seq>SEQ_GS_P_START) { sub_02004AB8(seq); *current=GBSounds_GetDSSeqNoByGBSeqNo(seq); }
    else *current=seq;
    GF_SetCurrentPlayingBGM(0);
}
u8 GF_GetPlayerNoBySeq(int seq) {
    if (!seq) return 255;
    const NNSSndSeqParam *p=NNS_SndArcGetSeqParam(seq);
    return p ? p->playerNo : 255;
}
void GF_SndHandleSetInitialVolume(s32 handle,s32 volume) {
    if (volume<0) volume=0;
    if (volume>127) volume=127;
    NNS_SndPlayerSetInitialVolume(GF_GetSoundHandle(handle),volume);
}
int sub_020378CC(void) { return sub_02034044(sub_0203993C()) ? ov00_021E7080() : 0; }
void sub_02005464(int seq,int handle) {
    const NNSSndSeqParam *p=NNS_SndArcGetSeqParam(seq);
    int volume;
    if (handle==1 || handle==8) volume=127;
    else { if (!p) return; volume=p->volume; }
    if (sub_020378CC()==1) GF_SndHandleSetInitialVolume(handle,volume/5);
}
