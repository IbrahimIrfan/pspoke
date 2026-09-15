# Build phases shared by scripts/platinum.sh and scripts/soulsilver.sh (sourced).
# Both games use one staged tree, .work/tree, laid out the way pspoke's build scripts expect.
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"
WORK="$ROOT/.work/tree"; T="$WORK/test_out"; LOGS="$WORK/logs"; U="$CACHE/upstream"
mkdir -p "$LOGS"
done_(){ [ -f "$WORK/.stamp-$1" ]; }; mark(){ touch "$WORK/.stamp-$1"; }

phase_base(){   # NitroSDK replacement + Platinum decompilation (SoulSilver reuses its SDK headers and services)
  done_ base && return
  log "Fetching pinned upstream sources"
  "$ROOT/scripts/fetch.sh" libntr libntrsystem libntrdwc libntrwifi libvct metang pokeplatinum
  log "Staging build tree in .work/tree"
  rm -rf "$T"; "$ROOT/scripts/stage.sh" "$WORK"
  for spec in "libntr native-graphics/libntr" "libntrsystem native-probe/libntrsystem" "libntrdwc native-probe/libntrdwc" "libntrwifi native-probe/libntrwifi" "libvct native-probe/libvct" "metang native-probe/metang"; do
    set -- $spec; rsync -a --exclude .git "$U/$1/" "$T/$2/"
  done
  rsync -a "$U/pokeplatinum/" "$T/native-probe/pokeplatinum/"   # keeps .git: gen-game-tables.py uses git ls-tree
  (cd "$T/native-probe/pokeplatinum" && patch -p1 -s < "$ROOT/patches/pokeplatinum/local-edits.patch")
  mark base
}

phase_generated(){
  done_ generated && return
  log "Generating headers from the decompilation"
  local N="$T/native-probe" P="$T/native-probe/pokeplatinum" G="$T/native-probe/generated"
  rm -rf "$G"; mkdir -p "$G"
  step genheaders   bash -c "cd '$N' && python3 genheaders.py"
  step source-meta  bash -c "cd '$N' && python3 gen-source-metadata.py"
  step game-tables  bash -c "cd '$N' && python3 gen-game-tables.py"
  step nitrogfx     make -C "$P/tools/nitrogfx"
  mkdir -p "$G/nitro/fx" "$G/res/fonts" "$G/res/graphics/battle/healthbox" "$G/res/words"
  step fx-const     python3 "$T/native-graphics/libntr/gen/nitro/fx/gen_fx_const.py" "$T/native-graphics/libntr/gen/nitro/fx/fx_const.csv" "$G/nitro/fx/fx_const.h"
  step embed-cursor bash -c "cd '$G/res/fonts' && '$P/tools/nitrogfx/nitrogfx' '$P/res/fonts/arrow_cursor.png' arrow_cursor.4bpp -embed sArrowCursorBitmap"
  step embed-health bash -c "cd '$G/res/graphics/battle/healthbox' && '$P/tools/nitrogfx/nitrogfx' '$P/res/graphics/battle/healthbox/healthbox_parts.png' healthbox_parts.4bpp -embed sHealthBoxPartsBitmap"
  printf '#define word_bank_o 0\n' > "$G/res/words/word_bank.naix"
  mark generated
}

phase_services(){
  done_ services && return
  log "Building PSP services (memory/register backing, threads, audio, graphics)"
  step backing       bash -c "cd '$T/native-probe/backing' && python3 generate.py && python3 generate_registers.py && make storage.o memory_layout.o device_registers.o && psp-ld -r storage.o memory_layout.o device_registers.o -o native-backing.o"
  step gfx-registers bash -c "cd '$T/native-graphics/alias-proof' && python3 generate_registers.py && make graphics_registers.o"
  step melon-2d      make -C "$T/native-render-opt" native_gpu.o GPU2D_Soft.o
  local s
  for s in "native-threads threads.o" "native-alarms alarms.o" "native-offline offline.o" "native-sdl-thread sdl_threads.o" "native-cadence cadence.o"; do
    set -- $s; step "${2%.o}" make -C "$T/$1" "$2"
  done
  step trainer-ai    bash -c "cd '$T/native-core-proof' && python3 rebuild.py"
  step audio         make -C "$T/native-audio-sound" sas_out.o audio_bank.o audio_loader.o audio_out.o audio_backend.o audio_seq.o audio_exchannel.o audio_channel.o audio_engine.o mic_pm.o os_sync.o
  mark services
}

phase_libraries(){
  done_ libraries && return
  log "Compiling SDK, network and internal libraries"
  step sdk-compile   bash -c "cd '$T/native-sdk-probe' && python3 compile.py"
  step sdk-archive   python3 "$ROOT/scripts/sdk_archive.py" "$T/native-sdk-probe"
  step sdk-filter    bash -c "cd '$T/native-audio-app' && python3 filter-sdk.py && cp -f libsdk-filtered.a libsdk-filtered.a.base"
  step network       bash -c "cd '$T/native-probe' && python3 compile-network.py"
  step internal      bash -c "cd '$T/native-probe' && python3 compile-internal.py"
  step particle-fix  bash -c "cd '$T/native-probe/spl-divzero' && python3 build.py --install"
  mark libraries
}

# stage_rom VAR PATH : fill a ROM placeholder in the staged tree (the ROM is only read, never copied into the repo).
fill_rom(){ { grep -rlI "@$1@" "$T" 2>/dev/null || true; } | while read -r f; do sed -i.bak "s#@$1@#$2#g" "$f" && rm -f "$f.bak"; done; }
dist_dir(){ local s=""; [ "$2" = 1 ] && s=-dev; echo "$ROOT/dist/$1$s/$3"; }
