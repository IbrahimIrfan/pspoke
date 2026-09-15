"""Original movement gates/forced movement scheduling; terrain/animation remain real dependencies."""
NAMES=['sub_0205CBEC','sub_0205CC4C','sub_0205CC74','sub_0205D004','sub_0205D07C','sub_0205D09C','sub_0205D0A8','sub_0205D190','sub_0205D1FC','sub_0205D2A0','sub_0205DA1C','sub_0205DA34']
SELECTION={'unk_0205CB48.s':NAMES}
EXPORTS={n:('int',['PlayerAvatar *','int']) for n in ['sub_0205CBEC','sub_0205D004','sub_0205D09C','sub_0205D0A8','sub_0205D190','sub_0205D2A0']}
EXPORTS.update({'sub_0205CC4C':('void',['PlayerAvatar *','int','uint16_t','uint16_t']),'sub_0205CC74':('void',['PlayerAvatar *']),'sub_0205D07C':('int',['PlayerAvatar *','int','int']),'sub_0205D1FC':('void',['PlayerAvatar *']),'sub_0205DA1C':('void',['PlayerAvatar *','LocalMapObject *','uint32_t']),'sub_0205DA34':('uint32_t',['PlayerAvatar *','LocalMapObject *','int'])})
SCALARS={
 'PlayerAvatar_GetMapObject':('LocalMapObject *',['PlayerAvatar *']),
 'MapObject_GetFieldSystem':('FieldSystem *',['LocalMapObject *']),
 'MapObject_AreBitsSetForMovementScriptInit':('int',['LocalMapObject *']),
 'MapObject_GetMovementCommand':('uint32_t',['LocalMapObject *']),
 'MapObject_GetNextFacingDirection':('uint32_t',['LocalMapObject *']),
 'MapObject_SetFlagsBits':('void',['LocalMapObject *','int']),
 'MapObject_ClearFlagsBits':('void',['LocalMapObject *','int']),
 'MapObject_SetHeldMovement':('void',['LocalMapObject *','uint32_t']),
 'PlayerAvatar_SetUnk28Unk2C':('void',['PlayerAvatar *','int32_t','int32_t']),
 'PlayerAvatar_SetUnk8':('void',['PlayerAvatar *','uint32_t']),
 'PlayerAvatar_GetMoveState':('uint32_t',['PlayerAvatar *']),
 'PlayerAvatar_GetState':('int32_t',['PlayerAvatar *']),
 'PlayerAvatar_GetUnk24':('int32_t',['PlayerAvatar *']),
 'PlayerAvatar_SetUnk24':('void',['PlayerAvatar *','int32_t']),
 'PlayerAvatar_SetMoveState':('void',['PlayerAvatar *','uint32_t']),
 'sub_0205DE64':('int',['uint32_t']), 'sub_0205D01C':('int',['PlayerAvatar *','int']),
 'sub_0205D240':('int',['PlayerAvatar *','int']), 'sub_0205D2D0':('void',['PlayerAvatar *','int']),
 'sub_020611F4':('uint32_t',['uint32_t']), 'sub_0206234C':('uint32_t',['int','uint32_t']),
 'sub_0206D494':('int',['FieldSystem *']),
}
for suffix in ['ClearFlag6','ClearUnk24ClearFlag2']:SCALARS['PlayerAvatar_'+suffix]=('void',['PlayerAvatar *'])
for suffix in ['CheckFlag6','CheckForcedMovement','CheckFlag7']:SCALARS['PlayerAvatar_'+suffix]=('int',['PlayerAvatar *'])
for suffix in ['SetFlag1','SetFlag2','SetFlag5','SetFlag7','SetForcedMovement']:SCALARS['PlayerAvatar_'+suffix]=('void',['PlayerAvatar *','int'])
for name in ['sub_0205DAA8','sub_0205DBF4','sub_0205DB68','sub_0205DCA0','sub_0205DCFC']:SCALARS[name]=('uint32_t',['PlayerAvatar *','LocalMapObject *','int'])
def configure(t):
 protos=['typedef struct PlayerAvatar PlayerAvatar;','typedef struct LocalMapObject LocalMapObject;','typedef struct FieldSystem FieldSystem;']
 for name,(ret,args) in SCALARS.items():
  protos.append('extern '+ret+' '+name+'('+','.join(args)+');')
  vals=[f'({ty})'+('(uintptr_t)' if '*' in ty else '')+f'r{i}' for i,ty in enumerate(args)]
  call=name+'('+','.join(vals)+');'
  t.CALLS[name]=( ('r0=(uint32_t)(uintptr_t)' if '*' in ret else 'r0=' if ret!='void' else '')+call,len(args))
 for name,(ret,args) in EXPORTS.items():
  vals=[f'r{i}' for i in range(len(args))]+['0']*(4-len(args))
  t.CALLS[name]=(('r0=' if ret!='void' else '')+'native_'+name+'('+','.join(vals)+');',len(args))
 t.INDIRECT_CALLS[('sub_0205D07C','r2')]='r0=((int(*)(PlayerAvatar*,int))(uintptr_t)r2)((PlayerAvatar*)(uintptr_t)r0,(int)r1);'
 return protos
