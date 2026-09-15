            switch (command.id) {
            case SND_COMMAND_START_SEQ:
                SND_StartSeq7((int)command.arg[0],
                             (const void *)command.arg[1],
                             command.arg[2], (struct SNDBankData *)command.arg[3]);
                break;
            case SND_COMMAND_STOP_SEQ:
                SND_StopSeq7((int)command.arg[0]);
                break;
            case SND_COMMAND_PREPARE_SEQ:
                SND_PrepareSeq7((int)command.arg[0],
                               (const void *)command.arg[1],
                               command.arg[2], (struct SNDBankData *)command.arg[3]);
                break;
            case SND_COMMAND_START_PREPARED_SEQ:
                SND_StartPreparedSeq7((int)command.arg[0]);
                break;
            case SND_COMMAND_PAUSE_SEQ:
                SND_PauseSeq7((int)command.arg[0], (BOOL)command.arg[1]);
                break;
            case SND_COMMAND_SKIP_SEQ:
                SND_SkipSeq7((int)command.arg[0], (u32)command.arg[1]);
                break;
            case SND_COMMAND_PLAYER_PARAM:
                SNDi_SetPlayerParam7((int)command.arg[0],
                                    command.arg[1], command.arg[2], (int)command.arg[3]);
                break;
            case SND_COMMAND_TRACK_PARAM:
                SNDi_SetTrackParam7((int)UNPACK_COMMAND(command.arg[0], 0, 24),
                                   command.arg[1],
                                   command.arg[2],
                                   command.arg[3], (int)UNPACK_COMMAND(command.arg[0], 24, 8));
                break;
            case SND_COMMAND_MUTE_TRACK:
                SND_SetTrackMute7((int)command.arg[0], command.arg[1], (SNDSeqMute) command.arg[2]);
                break;
            case SND_COMMAND_ALLOCATABLE_CHANNEL:
                SND_SetTrackAllocatableChannel7((int)command.arg[0], command.arg[1], command.arg[2]);
                break;
            case SND_COMMAND_PLAYER_LOCAL_VAR:
                SetLocal((int)command.arg[0],
                                           (int)command.arg[1], (s16)command.arg[2]);
                break;
            case SND_COMMAND_PLAYER_GLOBAL_VAR:
                SetGlobal((int)command.arg[0], (s16)command.arg[1]);
                break;
            case SND_COMMAND_START_TIMER:
                StartTimer(command.arg[0], command.arg[1], command.arg[2], command.arg[3]);
                break;
            case SND_COMMAND_STOP_TIMER:
                StopTimer(command.arg[0], command.arg[1], command.arg[2], command.arg[3]);
                break;
            case SND_COMMAND_SETUP_CAPTURE: Unsupported(command.id); break;
            case SND_COMMAND_SETUP_ALARM: Unsupported(command.id); break;
            case SND_COMMAND_CHANNEL_TIMER:
                SetChannelTimer((u32)command.arg[0], (int)command.arg[1]);
                break;
            case SND_COMMAND_CHANNEL_VOLUME:
                SetChannelVolume((u32)command.arg[0],
                                 (int)command.arg[1], (SNDChannelDataShift)command.arg[2]);
                break;
            case SND_COMMAND_CHANNEL_PAN:
                SetChannelPan((u32)command.arg[0], (int)command.arg[1]);
                break;
            case SND_COMMAND_SETUP_CHANNEL_PCM:
                SND_SetupChannelPcm7((int)UNPACK_COMMAND(command.arg[0], 0, 16),
                                    (const void *)(uintptr_t)command.arg[1],
                                    (SNDWaveFormat)UNPACK_COMMAND(command.arg[3], 24, 2),
                                    (SNDChannelLoop)UNPACK_COMMAND(command.arg[3], 26, 2),
                                    (int)UNPACK_COMMAND(command.arg[3], 0, 16),
                                    (int)UNPACK_COMMAND(command.arg[2], 0, 22),
                                    (int)UNPACK_COMMAND(command.arg[2], 24, 7),
                                    (SNDChannelDataShift)UNPACK_COMMAND(command.arg[2], 22, 2),
                                    (int)UNPACK_COMMAND(command.arg[0], 16, 16),
                                    (int)UNPACK_COMMAND(command.arg[3], 16, 7)
                                    );
                break;
            case SND_COMMAND_SETUP_CHANNEL_PSG:
                SND_SetupChannelPsg7((int)command.arg[0],
                                    (SNDDuty)command.arg[3],
                                    (int)UNPACK_COMMAND(command.arg[1], 0, 7),
                                    (SNDChannelDataShift)UNPACK_COMMAND(command.arg[1], 8, 2),
                                    (int)UNPACK_COMMAND(command.arg[2], 8, 16),
                                    (int)UNPACK_COMMAND(command.arg[2], 0, 7)
                                    );
                break;
            case SND_COMMAND_SETUP_CHANNEL_NOISE:
                SND_SetupChannelNoise7((int)command.arg[0],
                                      (int)UNPACK_COMMAND(command.arg[1], 0, 7),
                                      (SNDChannelDataShift)UNPACK_COMMAND(command.arg[1], 8, 2),
                                      (int)UNPACK_COMMAND(command.arg[2], 8, 16),
                                      (int)UNPACK_COMMAND(command.arg[2], 0, 7)
                                      );
                break;
            case SND_COMMAND_SURROUND_DECAY:
                SNDi_SetSurroundDecay7((int)command.arg[0]);
                break;
            case SND_COMMAND_MASTER_VOLUME:
                SetVolume((int)command.arg[0]);
                break;
            case SND_COMMAND_MASTER_PAN:
                SND_SetMasterPan7((int)command.arg[0]);
                break;
            case SND_COMMAND_OUTPUT_SELECTOR:
                SetOutput((SNDOutput)command.arg[0],
                                      (SNDOutput)command.arg[1],
                                      (SNDChannelOut)command.arg[2], (SNDChannelOut)command.arg[3]);
                break;
            case SND_COMMAND_LOCK_CHANNEL:
                SND_LockChannel7(command.arg[0], command.arg[1]);
                break;
            case SND_COMMAND_UNLOCK_CHANNEL:
                SND_UnlockChannel7(command.arg[0], command.arg[1]);
                break;
            case SND_COMMAND_STOP_UNLOCKED_CHANNEL:
                SND_StopUnlockedChannel7(command.arg[0], command.arg[1]);
                break;
            case SND_COMMAND_INVALIDATE_SEQ:
                SND_InvalidateSeq7((const void *)command.arg[0], (const void *)command.arg[1]);
                break;
            case SND_COMMAND_INVALIDATE_BANK:
                SND_InvalidateBank7((const void *)command.arg[0], (const void *)command.arg[1]);
                break;
            case SND_COMMAND_INVALIDATE_WAVE:
                SND_InvalidateWave((const void *)command.arg[0], (const void *)command.arg[1]);
                break;
            case SND_COMMAND_SHARED_WORK:
                SNDi_SharedWork = (SNDSharedWork *)command.arg[0];
                break;
            case SND_COMMAND_READ_DRIVER_INFO:
                ReadDriverInfo((SNDDriverInfo *) command.arg[0]);
                break;
            default: Unsupported(command.id);
            }

