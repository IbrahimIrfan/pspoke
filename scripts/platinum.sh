#!/usr/bin/env bash
# platinum.sh ROM DEV : build Pokémon Platinum for PSP from pinned upstream sources + pspoke port code + your ROM.
source "$(dirname "$0")/phases.sh"
ROM="$1"; DEV="${2:-0}"
phase_base; fill_rom PLATINUM_ROM "$ROM"; phase_generated; phase_services; phase_libraries

if ! done_ platinum-game; then
  log "Compiling Platinum (1023 source files; this takes a few minutes)"
  O="$T/native-audio-app/overlays"
  step ov-generate  bash -c "cd '$O' && python3 generate.py"
  step ov-build     bash -c "cd '$O' && python3 build.py"
  step qol-patch    bash -c "cd '$O/source' && patch -p1 -s < '$ROOT/patches/platinum/qol-overlay-sources.patch'"
  qol_header
  step qol-rebuild  bash -c "cd '$O' && python3 rebuild_obj.py text.c applications/bag/main.c game_options.c item.c overlay006/repel_step_update.c pokemon.c move_table.c applications/pokemon_summary_screen/main.c battle_sub_menus/battle_party.c"
  step ov-internal  bash -c "cd '$O' && python3 internal.py && cp -f libplatinum-overlays.a libplatinum-overlays.a.base"
  step ov-link      bash -c "cd '$O' && python3 gen-link.py"
  # The weakened fx matrix stubs give way to fx_mtx_native.o (battle particles call MTX_Scale43_), as in soulsilver.sh.
  step g3stack      bash -c "cd '$T/native-audio-app' && cp -f libsdk-filtered.a.base libsdk-filtered.a && python3 sdk-g3stack/rebuild.py && psp-objcopy --weaken-symbol=MTX_Copy33To43_ --weaken-symbol=MTX_Copy33To44_ --weaken-symbol=MTX_Copy43To44_ --weaken-symbol=MTX_Scale33_ --weaken-symbol=MTX_Scale43_ --weaken-symbol=MTX_Scale44_ --weaken-symbol=MTX_Transpose33_ --weaken-symbol=MTX_Transpose43_ --weaken-symbol=MTX_Transpose44_ libsdk-filtered.a"
  step tex-redirect bash -c "cd '$T/native-audio-app' && python3 opttex_redirect.py"
  mark platinum-game
fi

# The quality-of-life switches only touch these game files: rebuild them every time so a changed
# --no-* flag takes effect without a clean build.
qol_header
step qol-rebuild  bash -c "cd '$T/native-audio-app/overlays' && python3 rebuild_obj.py text.c applications/bag/main.c game_options.c item.c overlay006/repel_step_update.c pokemon.c move_table.c applications/pokemon_summary_screen/main.c battle_sub_menus/battle_party.c"
log "Linking Platinum EBOOT (DEV=$DEV)"
R="$T/native-stack-render"; A="$T/native-audio-app"
cp -f "$T/native-render-opt/native_gpu.o" "$T/native-render-opt/GPU2D_Soft.o" "$R/"
rm -f "$R"/{render-gu2d,g3_backend,frontend,g3_handler}.o "$R/libnative-render-gu2d.a"
step plat-renderer make -C "$R" DEV="$DEV"
rm -f "$A"/{frame,gameprof,input,main}.o "$A/EBOOT.PBP" "$A/native-app.elf" "$A/PARAM.SFO"
step plat-app      make -C "$A" DEV="$DEV" $(art_args platinum "$A")
OUT=$(dist_dir platinum "$DEV" NativePlatinum); mkdir -p "$OUT"; cp -f "$A/EBOOT.PBP" "$OUT/EBOOT.PBP"; printf 'display=%s\n' "${PSPOKE_DISPLAY:-nearest}" > "$OUT/pspoke.cfg"
python3 "$ROOT/scripts/check_native_pbp.py" "$OUT/EBOOT.PBP" > "$LOGS/audit-platinum.json" || die "EBOOT failed the PSP loader check (see $LOGS/audit-platinum.json)"
log "Done: $OUT/EBOOT.PBP"
