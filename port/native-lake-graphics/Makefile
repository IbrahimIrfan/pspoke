TARGET = native-app
OBJS = choice_trace.o ctrdg_absent.o search_disabled.o rowan_trace.o field_trace.o cadence_profile.o sdl_threads.o power.o audio_bank.o audio_loader.o audio_backend.o audio_seq.o audio_exchannel.o audio_channel.o audio_engine.o mic_pm.o os_sync.o offline.o gx_dma.o frame.o owner_lock.o threads.o alarms.o overlay.o main.o config.o platform.o input.o romfs.o backup.o backing.o graphics_registers.o trainer_ai.o mi_memory.o mi_dma.o
INCDIR = ../native-audio-probe/include overlays/include ../native-graphics/libntr/libraries/mi/src ../native-backup-probe/include ../native-fonts/include ../native-input/include ../native-startup/include ../native-probe/backing/include ../native-graphics/alias-proof/include ../native-probe/generated ../native-probe/pokeplatinum/include ../native-probe/libntrsystem/include ../native-graphics/libntr/include
CFLAGS = -DPM_KEEP_ASSERTS -O2 -G0 -std=gnu99 -DSDL_MAIN_HANDLED -DSDK_PORT -DSDK_X86 -DSDK_BUILD_PSP -DSDK_VERSION_MAJOR=4 -DSDK_TS -DSDK_4M -DSDK_FINALROM -DNNS_FINALROM -DPOKEPLATINUM_GENERATED_ENUM -ffunction-sections -fdata-sections -include ../native-probe/pokeplatinum/include/pch/global_pch.h
CFLAGS += $(EXTRA_CFLAGS) -DPSP_NATIVE_SCRIPTED_INPUT=1
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LIBS = libnative-render-frozen.a -Wl,--start-group overlays/libplatinum-overlays.a overlays/libinternal-overlays.a ../native-probe/libnetwork-probe.a libsdk-filtered.a -Wl,--end-group -lstdc++ -lpsprtc -lpspwlan -lpspgu -lpsppower -lpspaudio -lpspdebug -lpthread -lm
LDFLAGS = -Wl,--gc-sections,-Map,native-app.map
BUILD_PRX = 1
PSP_LARGE_MEMORY = 1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Native Platinum integration
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
romfs.o: services/romfs.c
	$(CC) $(CFLAGS) -c $< -o $@
backup.o: ../native-backup-probe/backup.c
	$(CC) $(CFLAGS) -c $< -o $@
backing.o: ../native-probe/backing/native-backing.o
	cp $< $@
graphics_registers.o: ../native-graphics/alias-proof/graphics_registers.o
	cp $< $@
trainer_ai.o: ../native-core-proof/trainer_ai.o
	cp $< $@
mi_memory.o: ../native-memory-probe/mi_memory.c
	$(CC) $(CFLAGS) -c $< -o $@
mi_dma.o: services/mi_dma.c
	$(CC) $(CFLAGS) -c $< -o $@

overlay.o: overlays/overlay.c overlays/ranges.h
	$(CC) $(CFLAGS) -c $< -o $@
LDFLAGS := $(subst $(PSPSDK)/lib/linkfile.prx,overlays/linkfile.prx,$(LDFLAGS)) $(shell cat overlays/constructors.flags)

threads.o: ../native-threads/threads.o
	cp $< $@
alarms.o: ../native-alarms/alarms.o
	cp $< $@

owner_lock.o: services/owner_lock.c
	$(CC) $(CFLAGS) -c $< -o $@

offline.o: ../native-offline/offline.o
	cp $< $@
gx_dma.o: services/gx_dma.c
	$(CC) $(CFLAGS) -c $< -o $@

audio_bank.o: ../native-sound-audio/audio_bank.o
	cp $< $@

audio_loader.o: ../native-sound-audio/audio_loader.o
	cp $< $@

audio_backend.o: ../native-sound-audio/audio_backend.o
	cp $< $@

audio_seq.o: ../native-sound-audio/audio_seq.o
	cp $< $@

audio_exchannel.o: ../native-sound-audio/audio_exchannel.o
	cp $< $@

audio_channel.o: ../native-sound-audio/audio_channel.o
	cp $< $@

audio_engine.o: ../native-sound-audio/audio_engine.o
	cp $< $@

mic_pm.o: ../native-sound-audio/mic_pm.o
	cp $< $@

os_sync.o: ../native-sound-audio/os_sync.o
	cp $< $@

power.o: services/power.c
	$(CC) $(CFLAGS) -c $< -o $@

sdl_threads.o: ../native-sdl-thread/sdl_threads.o
	cp $< $@

cadence.o: ../native-cadence/cadence.o
	cp $< $@

.PHONY: FORCE_NATIVE_CONFIG
FORCE_NATIVE_CONFIG:
frame.o: FORCE_NATIVE_CONFIG
$(TARGET).elf: | Makefile overlays/libplatinum-overlays.a overlays/libinternal-overlays.a libnative-render-frozen.a

# Static firmware-format audit catches the retail 80020148 section-bound failure.
.PHONY: check-native-format PARAM.SFO
all: check-native-format
check-native-format: EBOOT.PBP
	python3 ../../tools/check_native_pbp.py EBOOT.PBP

LDFLAGS += -Wl,--wrap=CTRDG_Init,--wrap=CommManager_InitializeSearchParty,--wrap=CommManager_GetAvailableConnections
