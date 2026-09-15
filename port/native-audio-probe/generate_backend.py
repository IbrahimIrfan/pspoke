from pathlib import Path
import re
b=Path(__file__).resolve().parent;r=b.parent/'native-graphics/libntr'
s=(r/'libraries/snd/src/snd_command.c').read_text();s=s[s.index('            switch (command.id)'):s.index('            command_p = command.next;')]
names=['StartSeq','StopSeq','PrepareSeq','StartPreparedSeq','PauseSeq','SkipSeq','SetTrackMute','SetTrackAllocatableChannel','SetupChannelPcm','SetupChannelPsg','SetupChannelNoise','SetMasterPan','LockChannel','UnlockChannel','StopUnlockedChannel','InvalidateSeq','InvalidateBank']
for n in names:s=re.sub(r'\bSND_'+n+r'\(', 'SND_'+n+'7(',s)
s=s.replace('SNDi_SetPlayerParam(', 'SNDi_SetPlayerParam7(').replace('SNDi_SetTrackParam(', 'SNDi_SetTrackParam7(').replace('SNDi_SetSurroundDecay(', 'SNDi_SetSurroundDecay7(')
s=s.replace('SND_SetPlayerLocalVariable(', 'SetLocal(').replace('SND_SetPlayerGlobalVariable(', 'SetGlobal(').replace('SND_SetMasterVolume(', 'SetVolume(').replace('SND_SetOutputSelector(', 'SetOutput(')
s=s.replace('(const void *)UNPACK_COMMAND(command.arg[1], 0, 27)','(const void *)(uintptr_t)command.arg[1]')
for case in ['SETUP_CAPTURE','SETUP_ALARM']:
 s=re.sub(r'case SND_COMMAND_'+case+r':.*?break;', 'case SND_COMMAND_'+case+': Unsupported(command.id); break;',s,flags=re.S)
s=s.replace('            }','            default: Unsupported(command.id);\n            }')
(b/'consumer_switch.inc').write_text(s)
s=(r/'libraries/sim/src/sim_audio.cpp').read_text().replace('(s_SIM_sndcnt[chNo]>>29)&0x3 == 3','((s_SIM_sndcnt[chNo]>>29)&0x3) == 3')
s=s.replace('ret = (s16)((double)ret * ((double)volume/128.0));', 'ret = (s16)((ret * volume) / 128);')
(b/'sim_audio.cpp').write_text(s + (b/'state_hash.inc').read_text())
