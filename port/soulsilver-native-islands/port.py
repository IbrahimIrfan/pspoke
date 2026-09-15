from pathlib import Path
import re,json,subprocess
B=Path(__file__).resolve().parent;C=B.parent/'soulsilver-native-core'
replacements={
'unk_02054648':{
'sub_020547A4':'''static fx32 sub_020547A4(FieldSystem *fieldSystem, fx32 refHeight, fx32 xFx32, fx32 zFx32, u8 *outSelector) {
    // ARM's sign correction followed by ASR16 implements truncation toward zero.
    BOOL hit = ov01_021F654C(fieldSystem->mapLoadManager, xFx32 / 65536, zFx32 / 65536, NULL);
    if (outSelector != NULL) *outSelector = hit != 0;
    return 0;
}''',
'sub_02054DC8':'''void sub_02054DC8(int idx, int width, VecFx32 *out) {
    // Preserve ARM wrapping shifts and leave Y untouched.
    GF_ASSERT(width != 0);
    s64 q = (s64)idx / width;
    s64 r = (s64)idx % width;
    out->x = (fx32)(0x100000u + ((u32)(u16)r << 21));
    out->z = (fx32)(0x100000u + ((u32)(u16)q << 21));
}'''},
'unk_020773AC':{
'sub_0207741C':'''static void sub_0207741C(void) {
    GfGfx_EngineATogglePlanes(1, 1);
    reg_G2_BG0CNT = (reg_G2_BG0CNT & ~3u) | 1u;
    reg_G3X_DISP3DCNT &= 0xcffdu;
    reg_G3X_DISP3DCNT = (reg_G3X_DISP3DCNT & 0xcfffu) | 0x10u;
    reg_G3X_DISP3DCNT &= 0xcffbu;
    reg_G3X_DISP3DCNT = (reg_G3X_DISP3DCNT & 0xcfffu) | 8u;
    reg_G3X_DISP3DCNT &= 0xcfdfu;
    G3X_SetFog(0, 0, 0, 0);
    G3X_SetClearColor(0, 0, 0x7fff, 0x3f, 0);
    G3_ViewPort(0, 0, 255, 191);
}''',
'sub_020775C4':'''static void sub_020775C4(SPLEmitter *emitter) {
    emitter->emtr_pos.x = emitter->p_res->p_base->pos.x;
    emitter->emtr_pos.y = (fx32)((u32)emitter->p_res->p_base->pos.y + 0x560u);
    emitter->emtr_pos.z = emitter->p_res->p_base->pos.z;
}'''} }
for stem,funcs in replacements.items():
 s=(C/'src'/(stem+'.c')).read_text()
 for name,body in funcs.items():
  pattern=r'(?:static )?asm [^\n]*\b'+name+r'\([^\n]*\) \{.*?^}'
  s,n=re.subn(pattern,lambda m:body,s,count=1,flags=re.S|re.M);assert n==1,name
 if stem=='unk_020773AC':
  s=s.replace('G2x_SetBlendAlpha_(0x04000050,','G2x_SetBlendAlpha_((u32)REG_BLDCNT_ADDR,').replace('G2x_SetBlendAlpha_(0x04001050,','G2x_SetBlendAlpha_((u32)REG_DB_BLDCNT_ADDR,')
  s+='\n#ifdef SS_NATIVE_ISLAND_TEST\nvoid SS_TestParticle(SPLEmitter *e) { sub_020775C4(e); }\nvoid SS_TestGraphics(void) { sub_0207741C(); }\n#endif\n'
 else:s+='\n#ifdef SS_NATIVE_ISLAND_TEST\nfx32 SS_TestHeight(FieldSystem *f, fx32 x, fx32 z, u8 *o) { return sub_020547A4(f, 123, x, z, o); }\n#endif\n'
 p=B/(stem+'.c');p.write_text(s)
 cmd=json.loads((C/'compile-command.json').read_text())+['-DSS_NATIVE_ISLAND_TEST','-c',str(p),'-o',str(B/(stem+'.o'))]
 r=subprocess.run(cmd,text=True,capture_output=True);(B/(stem+'.log')).write_text(r.stdout+r.stderr);print(stem,r.returncode,r.stderr[-2000:])
