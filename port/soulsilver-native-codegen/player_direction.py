"""D-pad axis selection and original diagonal direction priority."""
SELECTION={'unk_0205CB48.s':['sub_0205DD9C','sub_0205DDB8','sub_0205DDD4']}
EXPORTS={'sub_0205DD9C':('int',['uint32_t']),'sub_0205DDB8':('int',['uint32_t']),'sub_0205DDD4':('int',['PlayerAvatar *','uint16_t','uint16_t'])}
def configure(t):
 protos=['typedef struct PlayerAvatar PlayerAvatar;']
 for name,ret in [('PlayerAvatar_GetNextFacingDirection','uint32_t'),('PlayerAvatar_GetUnk28','int32_t'),('PlayerAvatar_GetUnk2C','int32_t')]:
  protos.append('extern '+ret+' '+name+'(PlayerAvatar*);')
  t.CALLS[name]=('r0='+name+'((PlayerAvatar*)(uintptr_t)r0);',1)
 for name,(ret,args) in EXPORTS.items():
  vals=[f'r{i}' for i in range(len(args))]+['0']*(4-len(args))
  t.CALLS[name]=('r0=native_'+name+'('+','.join(vals)+');',len(args))
 return protos
