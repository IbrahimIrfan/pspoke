#!/usr/bin/env bash
# soulsilver.sh ROM DEV : build Pokémon SoulSilver for PSP. Reuses the SDK/services built for the shared tree.
source "$(dirname "$0")/phases.sh"
ROM="$1"; DEV="${2:-0}"
phase_base; phase_generated; phase_services; phase_libraries
C="$T/soulsilver-native-core"; P="$C/nitromain-perf"

if ! done_ soulsilver-prepare; then
  log "Preparing the HeartGold/SoulSilver decompilation"
  "$ROOT/scripts/fetch.sh" pokeheartgold-slop pokeheartgold
  rsync -a --exclude .git "$U/pokeheartgold-slop/" "$T/soulsilver-research/pokeheartgold-slop/"
  rsync -a --exclude .git "$U/pokeheartgold/" "$T/soulsilver-native-audit/pokeheartgold/"
  fill_rom SOULSILVER_ROM "$ROM"
  step ss-member-maps  bash -c "cd '$T/soulsilver-native-assets' && python3 audit_members.py"
  step ss-prepare      bash -c "cd '$C' && python3 prepare.py && python3 finish_headers.py && python3 final_compat.py"
  step ss-patch        bash -c "cd '$C' && for d in src include game-include; do patch -p1 -s < '$ROOT/patches/soulsilver/'\$d.patch; done"
  mark soulsilver-prepare
fi

if ! done_ soulsilver-game; then
  log "Compiling SoulSilver (529 game files + translated code; this takes a few minutes)"
  step ss-core         bash -c "cd '$C' && python3 crossprobe.py && python3 archive.py"
  # GCC's strict-aliasing optimisation deletes the cut-in clamps in this file (Waterfall/field-move cut-ins never end).
  step ss-aliasing-fix bash -c "cd '$C' && python3 -c \"import json,subprocess;subprocess.run(json.load(open('compile-command.json'))+['-fno-strict-aliasing','-c','src/overlay_02_02248728.c','-o','objects/overlay_02_02248728.o'],check=True)\" && psp-ar r libsoulsilver-c.a objects/overlay_02_02248728.o"
  step ss-data         bash -c "cd '$T/soulsilver-native-data' && python3 build_data.py"
  step ss-overlays     bash -c "cd '$T/soulsilver-native-overlays' && python3 build_registry.py"
  step ss-codegen      bash -c "cd '$T/soulsilver-native-codegen' && python3 library.py"
  step ss-maploader    bash -c "cd '$T/soulsilver-native-play/maploader' && python3 port.py --compile"
  step ss-movement     bash -c "cd '$T/soulsilver-native-player-movement' && make movement.o terrain.o control.o && psp-ar rcs libss-player-movement.a movement.o terrain.o control.o"
  step ss-menu-sprites bash -c "cd '$T/soulsilver-native-menu-sprites' && make progress.o graphics.o && psp-ar rcs libss-menu-sprites.a progress.o graphics.o"
  step ss-window       bash -c "cd '$T/soulsilver-native-window' && python3 port.py"
  step ss-sound        bash -c "cd '$T/soulsilver-native-sound-helpers' && make helpers.o && psp-ar rcs libss-sound-helpers.a helpers.o"
  step ss-fade         make -C "$T/soulsilver-native-assets/fade-port/wipe-candidate" libss-native-fade.a
  step ss-wfc          bash -c "cd '$T/soulsilver-native-islands' && make wfc_startup.o && psp-ar rcs libss-wfc-startup.a wfc_startup.o"
  step ss-particles    bash -c "cd '$T/soulsilver-native-particles' && python3 build.py"
  step ss-billboards   bash -c "cd '$T/soulsilver-native-billboards' && python3 build.py && make bootstrap.o"
  # Service objects shared with Platinum (same sources and flags), plus SoulSilver's own sound backend.
  step ss-services     make -C "$T/native-audio-app" owner_lock.o backup.o audio_bank.o audio_loader.o audio_seq.o audio_exchannel.o audio_channel.o offline.o gx_dma.o threads.o alarms.o mi_memory.o mi_dma.o backing.o graphics_registers.o power.o mic_pm.o os_sync.o sdl_threads.o ctrdg_absent.o
  step ss-sound-engine make -C "$T/native-sound-audio" audio_backend.o audio_engine.o sas_out.o
  for o in owner_lock backup audio_bank audio_loader audio_seq audio_exchannel audio_channel offline gx_dma threads alarms mi_memory mi_dma backing graphics_registers power mic_pm os_sync sdl_threads ctrdg_absent; do cp -f "$T/native-audio-app/$o.o" "$P/"; done
  cp -f "$T/native-sound-audio/audio_backend.o" "$T/native-sound-audio/audio_engine.o" "$T/native-sound-audio/sas_out.o" "$P/"
  cp -f "$T/soulsilver-native-billboards/bootstrap.o" "$P/billboard_bootstrap.o"
  cp -f "$T/native-audio-app/libsdk-filtered.a.base" "$P/libsdk-filtered-ss.a"
  step ss-sdk          psp-objcopy --weaken-symbol=MTX_Copy33To43_ --weaken-symbol=MTX_Copy33To44_ --weaken-symbol=MTX_Copy43To44_ --weaken-symbol=MTX_Scale33_ --weaken-symbol=MTX_Scale43_ --weaken-symbol=MTX_Scale44_ --weaken-symbol=MTX_Transpose33_ --weaken-symbol=MTX_Transpose43_ --weaken-symbol=MTX_Transpose44_ "$P/libsdk-filtered-ss.a"
  step ss-overlay-obj  bash -c "cd '$P' && make overlay.o"
  step ss-cull-vectors bash -c "cd '$P' && python3 generate_culling_vectors.py"
  mark soulsilver-game
fi

log "Linking SoulSilver EBOOT (DEV=$DEV)"
F="$T/soulsilver-native-assets/render-fastcompare"
cp -f "$T/native-render-opt/native_gpu.o" "$T/native-render-opt/GPU2D_Soft.o" "$F/"
rm -f "$F"/{render-gu2d,g3_backend,frontend,g3_handler,window_adapter}.o "$F/libnative-render-gu2d.a"
step ss-renderer     bash -c "cd '$F' && make DEV='$DEV' libnative-render-gu2d.a window_adapter.o"
cp -f "$F/libnative-render-gu2d.a" "$P/libss-render.a"; cp -f "$F/window_adapter.o" "$P/window_adapter.o"
rm -f "$P"/{frame,main,osk}.o "$P"/diag_*.o "$P/ss-native-main.elf" "$P/ss-native-main.prx" "$P/EBOOT.PBP" "$P/PARAM.SFO"
step ss-app          make -C "$P" PSP_NATIVE_PROBE_FRAMES=0 PSP_NATIVE_PLAY=1 DEV="$DEV"
OUT=$(dist_dir soulsilver "$DEV" NativeSoulSilver); mkdir -p "$OUT"; cp -f "$P/EBOOT.PBP" "$OUT/EBOOT.PBP"
python3 "$ROOT/scripts/check_native_pbp.py" "$OUT/EBOOT.PBP" > "$LOGS/audit-soulsilver.json" || die "EBOOT failed the PSP loader check (see $LOGS/audit-soulsilver.json)"
log "Done: $OUT/EBOOT.PBP"
