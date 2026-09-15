SELECTION={'unk_02004A44.s':['sub_02005BFC','sub_02005990','GF_SndHandleMoveVolume','GF_GetCurrentPlayingBGM','GF_SetVolumeBySeqNo','SoundSys_GetGBSoundsState','GBSounds_GetGBSeqNoByDSSeqNo','GBSounds_SetAllocatableChannels','GF_SndPlayerCountPlayingSeqByPlayerNo','Sound_SetSceneAndPlayBGM','Sound_SetScene','sub_02005AF8','sub_020058F4','sub_020052E4','sub_02005060','sub_02004B24','sub_02005328','GF_SetCurrentPlayingBGM']}
EXPORTS={
 'sub_02005990':('void',['int']),
 'GF_SndHandleMoveVolume':('void',['int','int','int']),
 'sub_02005BFC':('int',[]),
 'GF_GetCurrentPlayingBGM':('uint16_t',[]),
 'GF_SetVolumeBySeqNo':('void',['uint16_t','uint16_t']),
 'SoundSys_GetGBSoundsState':('int',[]), 'GBSounds_GetGBSeqNoByDSSeqNo':('uint16_t',['uint16_t']), 'GBSounds_SetAllocatableChannels':('void',[]),
 'GF_SndPlayerCountPlayingSeqByPlayerNo':('uint32_t',['uint32_t']),
 'Sound_SetSceneAndPlayBGM':('void',['uint8_t','uint16_t','int']),
 'Sound_SetScene':('void',['int']), 'sub_02005AF8':('void',['int']),
 'sub_020058F4':('int',[]), 'sub_020052E4':('void',['int','uint16_t','int']),
 'sub_02005060':('void',['int']), 'sub_02004B24':('void',['int']),
 'sub_02005328':('int',['int']), 'GF_SetCurrentPlayingBGM':('void',['uint16_t']),
}
SCALARS={
 'NNS_SndPlayerMoveVolume':('void',['void *','int','int']), 'GF_SndWorkSetGbSoundsVolume':('void',['uint8_t']),
 'GF_GetPlayerNoBySeq':('uint8_t',['int']), 'GF_GetSndHandleByPlayerNo':('int',['int']), 'GF_SndHandleSetInitialVolume':('void',['int','int']),
 'GF_GetSoundHandle':('void *',['int']), 'NNS_SndPlayerSetTrackAllocatableChannel':('void',['void *','uint16_t','uint32_t']),
 'NNS_SndPlayerCountPlayingSeqByPlayerNo':('int',['int']),
 'GF_SdatGetAttrPtr':('void *',['uint32_t']),
 'GetSoundDataPointer':('void *',[]),
 'GF_SndSetAllocatableChannelForBGMPlayer':('void',['uint32_t']),
 'sub_02005910':('void',['int']), 'sub_02005908':('int',[]),
 'NNS_SndCaptureIsActive':('int',[]),'Sound_Stop':('void',[]),
 'GF_Snd_LoadState':('void',['int']),'GF_Snd_SaveState':('int',['int *']),
 'PlayBGM':('int',['uint16_t']),
 'GF_Snd_LoadGroup':('int',['int']), 'GF_Snd_LoadSeqEx':('int',['int','uint32_t']),
 'GF_Snd_LoadBank':('int',['int']), 'GF_Snd_LoadWaveArc':('int',['int']),
 'sub_0200508C':('void',['uint16_t','int']), 'sub_02005228':('void',['uint16_t','int']),
 'sub_02005260':('void',['uint16_t','int']), 'sub_02005280':('void',['uint16_t','int']),
 'sub_020052A4':('void',['uint16_t','int']), 'sub_020052C8':('void',['int']),
}
def configure(t):
 protos=[]
 for name,(ret,args) in SCALARS.items():
  protos.append('extern '+ret+' '+name+'('+(','.join(args) or 'void')+');')
  vals=[f'({ty})'+('(uintptr_t)' if '*' in ty else '')+f'r{i}' for i,ty in enumerate(args)]
  call=name+'('+','.join(vals)+');'
  t.CALLS[name]=( ('r0=(uint32_t)(uintptr_t)' if '*' in ret else 'r0=' if ret!='void' else '')+call,len(args))
 for name,(ret,args) in EXPORTS.items():
  vals=[f'r{i}' for i in range(len(args))]+['0']*(4-len(args))
  t.CALLS[name]=(('r0=' if ret!='void' else '')+'native_'+name+'('+','.join(vals)+');',len(args))
 return protos
