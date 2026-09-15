from pathlib import Path
b=Path(__file__).resolve().parent
p=b/'game-include/unk_02034354.h';p.write_text(p.read_text().replace('void sub_02034638(void);','int sub_02034638(void);'))
p=b/'include/nitro/hw/common/io_reg.h';p.write_text('#include <nitro/hw/ARM9/ioreg.h>\n')
# The source computes bit15 from the literal DS register address; keep that original
# constant rather than arithmetic on the portable pointer macro.
p=b/'src/overlay_44_0222CDAC.c';p.write_text(p.read_text().replace('REG_POWCNT_ADDR / 2048','0x04000304u / 2048'))
p=b/'src/berry_pots_app_tasks.c';p.write_text(p.read_text().replace('void ov17_02203928(', 'static void ov17_02203928(').replace('static static void','static void'))
p=b/'src/battle/battle_command.c';s=p.read_text();start=s.index('#define CP_SQRT_32BIT_MODE');end=s.index('\n}',s.index('static inline u32 CP_GetSqrtResult32',start))+2;s=s[:start]+ '/* Portable SDK owns equivalent CP square-root helpers. */\n'+s[end:];p.write_text(s)
