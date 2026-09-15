"""Audited main-loop communication wrappers; active network callees stay external."""
SELECTION={
 'unk_02037C94.s':['sub_020399B8','sub_0203817C','sub_02039998','sub_0203993C','sub_02039A00','sub_02039918','sub_020399A4'],
 'unk_02035900.s':['sub_02036144'],
 'unk_02034B0C.s':['sub_020355C8','sub_02035650'],
}
EXPORTS={name:('void',[]) if name in ['sub_0203817C','sub_02039A00'] else ('void',['uint32_t']) if name in ['sub_020355C8','sub_020399A4'] else ('int',[]) for names in SELECTION.values() for name in names}
# No entry args are consumed by these original private functions. Scalar return
# values are used only where the original caller compares r0.
SCALARS={
 'sub_0201A79C':('int',[]),'sub_02034044':('int',['int']),
 'sub_02034084':('int',['int']), 'sub_02035E9C':('void',[]),
 'sub_02035F4C':('void',[]),'sub_02035FF0':('int',[]),
 'sub_0203611C':('void',[]),'sub_02036298':('void',[]),
 'sub_0203667C':('void',[]),'sub_02036BE4':('void',[]),
 'sub_020372E4':('void',[]),'sub_02037334':('void',[]),
 'sub_020373B4':('int',['uint16_t']),'sub_0203769C':('int',[]),
 'sub_02037ADC':('void',[]),'sub_0203540C':('void',['uint32_t']),
 'ov00_021EC9D4':('int',[]),'WM_GetLinkLevel':('int',[]),
 'sub_0203A930':('void',['int']),'sub_02037700':('int',[]),
 'sub_020393C8':('int',[]),'sub_020395B0':('int',[]),
 'sub_020397FC':('int',[]),'Sound_Stop':('void',[]),
 'Save_Cancel':('void',['void *']),
}
def configure(t):
 protos=[]
 for name,(ret,args) in SCALARS.items():
  protos.append('extern '+ret+' '+name+'('+(','.join(args) or 'void')+');')
  vals=[f'({ty})'+('(uintptr_t)' if '*' in ty else '')+f'r{i}' for i,ty in enumerate(args)]
  expression=name+'('+','.join(vals)+');'
  t.CALLS[name]=(('r0=' if ret!='void' else '')+expression,len(args))
 for name,(ret,args) in EXPORTS.items():
  vals=[f'r{i}' for i in range(len(args))]+['0']*(4-len(args))
  t.CALLS[name]=(('r0=' if ret!='void' else '')+'native_'+name+'('+','.join(vals)+');',len(args))
 t.INDIRECT_CALLS[('sub_0203817C','r0')]='((void(*)(void))(uintptr_t)r0)();'
 return protos
