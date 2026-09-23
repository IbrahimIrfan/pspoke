#!/usr/bin/env bash
# pspoke regression tests: build each game, audit the EBOOT against the PSP loader limits, then replay
# recorded scenarios in headless PPSSPP and check the game's own log. See tests/README.md.
#
#   tests/run.sh --platinum-rom P.nds --soulsilver-rom S.nds        # everything (about 25 minutes)
#   tests/run.sh --soulsilver-rom S.nds --quick                     # smoke scenarios only
#   tests/run.sh --soulsilver-rom S.nds --only qol-repel-yes,pc     # named scenarios
#   tests/run.sh --list                                             # scenario names
#   options: --skip-build (use the existing .work tree), PPSSPP_HEADLESS=/path/to/PPSSPPHeadless (emulator scenarios)
source "$(dirname "$0")/../scripts/phases.sh"
PLAT_ROM="${PLATINUM_ROM:-}"; SS_ROM="${SOULSILVER_ROM:-}"; PLAT_SAVE="${PLATINUM_SAVE:-}"; QUICK=0; ONLY=""; SKIP_BUILD=0; LIST=0
while [ $# -gt 0 ]; do case "$1" in
  --platinum-rom) PLAT_ROM="$2"; shift 2;;
  --soulsilver-rom) SS_ROM="$2"; shift 2;;
  --platinum-save) PLAT_SAVE="$2"; shift 2;;   # a copy of a save in the overworld (never modified); see tests/README.md
  --quick) QUICK=1; shift;;
  --only) ONLY=",$2,"; shift 2;;
  --skip-build) SKIP_BUILD=1; shift;;
  --list) LIST=1; shift;;
  *) die "unknown option $1 (see the top of tests/run.sh)";;
esac; done
PPSSPP="${PPSSPP_HEADLESS:-}"
OUT="$ROOT/.work/tests"; mkdir -p "$OUT"; SUMMARY="$OUT/summary.txt"; : > "$SUMMARY"
PASSED=0; FAILED=0
# Lines that mean the game trapped, asserted or lost an overlay. PPSSPP mirrors the game's stdout into run.log.
TRAPS='FATAL\]|SS-MISSING|GF_ASSERT|SIGSEGV|invalid input range|LIST-VERIFY\] MISMATCH|[Ii]nvalid address|Jumped to invalid'

result(){ # result PASS|FAIL NAME DETAIL
  if [ "$1" = PASS ]; then PASSED=$((PASSED+1)); printf '    \033[32mPASS\033[0m %-22s %s\n' "$2" "$3"; else FAILED=$((FAILED+1)); printf '    \033[31mFAIL\033[0m %-22s %s\n' "$2" "$3"; fi
  echo "$1 $2 $3" >> "$SUMMARY"; }
want(){ [ -z "$ONLY" ] || [[ "$ONLY" == *",$1,"* ]]; }
# judge NAME LOG CHECK... : traps=0 and a clean bounded exit, plus optional overlay:<id> / log:<text> checks.
judge(){ local name=$1 log=$2; shift 2; local why="" n
  n=$(grep -a -c -E "$TRAPS" "$log" || true); [ "$n" = 0 ] || why="$why traps=$n"
  grep -a -q 'clean bounded probe exit' "$log" || why="$why no-clean-exit"
  for c in "$@"; do case "$c" in
    overlay:*) grep -a -q -E "SS-OVERLAY\] native load id=${c#overlay:}([^0-9]|$)" "$log" || why="$why missing-overlay-${c#overlay:}";;
    log:*) grep -a -q -F -- "${c#log:}" "$log" || why="$why missing-log:'${c#log:}'";;
    belts:*) python3 "$ROOT/tests/platinum/count_belt_pixels.py" "$OUT/$name.png" >/dev/null || why="$why belts-not-drawn";;
  esac; done
  local fps; fps=$(grep -a -o 'average_fps=[0-9.]*' "$log" | tail -1 | cut -d= -f2 || true); [ -z "$fps" ] || fps="emulator fps $fps"
  if [ -z "$why" ]; then result PASS "$name" "$fps"; else result FAIL "$name" "${why# } (see .work/tests/$name.log and .png)"; fi; }
# keep_run NAME APPDIR : move the newest probe run's log and screenshot into .work/tests and delete the run.
keep_run(){ local r; r=$(ls -td "$2"/runs/2*/ 2>/dev/null | head -1) || true
  [ -n "$r" ] || { : > "$OUT/$1.log"; return; }
  cp -f "$r/run.log" "$OUT/$1.log"; cp -f "$r/screen.png" "$OUT/$1.png" 2>/dev/null || true
  cat "$r"/memstick/PSP/GAME/*/native-memlog.txt >> "$OUT/$1.log" 2>/dev/null || true; rm -rf "$r"; }

### SoulSilver ###############################################################################################
P="$T/soulsilver-native-core/nitromain-perf"
SS_BUILT=""
# ss_build TAG MAKEVARS... : relink the app as a bounded test build (the shipping link, PSP_NATIVE_PROBE_FRAMES=0,
# ignores probe-frame-limit.txt). Diagnostic make variables (WARP_TO, GIVE_SPECIES, ...) are documented in the Makefile.
ss_build(){ local tag=$1; shift; [ "$SS_BUILT" = "$tag" ] && return
  printf '    %-28s' "relink soulsilver ($tag)"
  if (cd "$P" && rm -f frame.o main.o osk.o diag_*.o ss-native-main.elf ss-native-main.prx EBOOT.PBP PARAM.SFO \
      && make PSP_NATIVE_PROBE_FRAMES=100000 PSP_NATIVE_PLAY=1 DEV=0 "$@") > "$OUT/build-soulsilver-$tag.log" 2>&1; then echo ok; SS_BUILT=$tag
  else echo "FAILED (see .work/tests/build-soulsilver-$tag.log)"; tail -5 "$OUT/build-soulsilver-$tag.log"; exit 1; fi; }
# ss_case NAME FIXTURE INPUT EXTRA_FRAMES CHECK... : replay INPUT from FIXTURE and stop EXTRA_FRAMES after its last event.
ss_case(){ local name=$1 fx=$2 in=$3 extra=$4; shift 4; want "$name" || return 0
  local frames=$(( $(tail -1 "$P/$in" | awk '{print $1}') + extra ))
  (cd "$P" && python3 run_probe.py --rom "$SS_ROM" --ppsspp "$PPSSPP" --input-script "$in" --fixture "runs/fixtures/$fx.sav" \
     --frames "$frames" --seconds 600) > "$OUT/$name.probe.txt" 2>&1 || true
  keep_run "$name" "$P"; judge "$name" "$OUT/$name.log" "$@"; }
ss_group_wanted(){ for n in "$@"; do want "$n" && return 0; done; return 1; }

ss_tests(){
  mkdir -p "$P/runs/fixtures"; cp -f "$ROOT"/tests/fixtures/soulsilver/*.sav "$P/runs/fixtures/"
  cp -f "$ROOT/port/soulsilver-native-core/nitromain-perf/run_probe.py" "$P/"   # staged copy may predate fixes
  ss_build default
  ss_case smoke            violet-center-postrod input-smoke-clean.txt 600
  [ "$QUICK" = 1 ] && return
  ss_case pc               violet-center-postrod input-reg-pc.txt      600
  ss_case catch            route31-west          input-catch5.txt      900 overlay:12 "log:[SS-QOL] move buffs: Cut 60/100 Rock Smash 60/"
  ss_case easychat         violet-center-postrod input-easychat2.txt   600
  ss_case pokedex          violet-center-postrod input-dexmid2.txt     600
  ss_case apricorn         violet-center-postrod input-key3.txt        600
  ss_case vs-recorder      violet-center-postrod input-vsr3.txt        600
  ss_case trainer-card     violet-center-postrod input-tcE.txt         600
  ss_case options          violet-center-postrod input-tc2.txt         600
  ss_case options-confirm  violet-center-postrod input-fx-confirm.txt  120
  ss_case options-quit     violet-center-postrod input-fx-quit.txt     120
  ss_case options-b        violet-center-postrod input-fx-b.txt        120
  ss_case options-scene    violet-center-postrod input-fx-scene.txt    120
  ss_case options-nochange violet-center-postrod input-fx-nochange.txt 120
  ss_case options-after    violet-center-postrod input-fx-after.txt    120
  if ss_group_wanted geonet; then ss_build geonet WARP_TO=207,9,11
    ss_case geonet         violet-center-postrod input-geo3.txt        600 overlay:69 "log:downscaled 1024x512 -> 512x256"; fi
  if ss_group_wanted gym-pryce; then ss_build pryce PARTY_LEVEL=100 TEACH_MOVES=0:85,0:53,0:94,0:247 NO_WILD=1
    ss_case gym-pryce      diag-mahogany-gym-pryce input-perfwp2.txt   120 overlay:12; fi
  if ss_group_wanted rocket-radio-tower; then ss_build rocket WARP_TO=186,16,7 NO_WILD=1
    ss_case rocket-radio-tower diag-radio-takeover input-rocket117-loop.txt 300 overlay:12 overlay:117; fi
  # Quality-of-life features (default build switches; these fail on a --no-trade-evos / --no-repel-prompt build).
  if ss_group_wanted qol-friendship-evo; then ss_build ching GIVE_SPECIES=433 GIVE_LEVEL=10 GIVE_FRIENDSHIP=220 GIVE_ITEM_IDS=50:2 NO_WILD=1
    ss_case qol-friendship-evo diag-azalea-saved input-qol-ching.txt 120 overlay:15; fi
  if ss_group_wanted qol-trade-item-evo; then ss_build onix GIVE_SPECIES=95 GIVE_LEVEL=20 GIVE_ITEM_IDS=233:1 NO_WILD=1
    ss_case qol-trade-item-evo diag-azalea-saved input-qol-onix.txt  120 overlay:15; fi
  if ss_group_wanted qol-trade-level-evo; then ss_build kadabra GIVE_SPECIES=64 GIVE_LEVEL=35 GIVE_ITEM_IDS=50:2 NO_WILD=1
    ss_case qol-trade-level-evo diag-azalea-saved input-qol-kad.txt 120 overlay:15; fi
  if ss_group_wanted qol-repel-yes qol-repel-no; then ss_build repel REPEL_STEPS=3 REPEL_FULL=100 GIVE_ITEM_IDS=79:2 NO_WILD=1
    ss_case qol-repel-yes  diag-azalea-saved input-qol-repyes.txt     120 "log:SS-QOL] repel reuse YES: item 79 used, steps 100"
    ss_case qol-repel-no   diag-azalea-saved input-qol-repno.txt      120 "log:SS-QOL] repel reuse NO: item 79 kept"; fi
}

### Platinum #################################################################################################
A="$T/native-audio-app"
# plat_build TAG FRAMES [SCRIPT.h] [MAKEVARS...] : relink with a frame bound (Platinum reads no probe-frame-limit.txt;
# the bound is compiled in), optionally a scripted input header (tests/platinum/*.h) and diagnostic make variables
# such as WARP_TO=<map>,<x>,<z> (see port/native-audio-app/diag_warp.c).
plat_build(){ local tag=$1 frames=$2 script=${3:-}; shift 2; [ $# -gt 0 ] && shift; local cf="-DPSP_NATIVE_PROBE_FRAMES=$frames"
  [ -z "$script" ] || cf="$cf -DPSP_NATIVE_SCRIPTED_INPUT -DPSP_NATIVE_INPUT_SCRIPT=\\\"$ROOT/tests/platinum/$script\\\""
  printf '    %-28s' "relink platinum ($tag)"
  if (cd "$A" && rm -f frame.o input.o osk.o naming_osk.o main.o diag_warp.o native-app.elf native-app.prx EBOOT.PBP PARAM.SFO \
      && make DEV=0 "$@" EXTRA_CFLAGS="$cf") > "$OUT/build-platinum-$tag.log" 2>&1; then echo ok
  else echo "FAILED (see .work/tests/build-platinum-$1.log)"; tail -5 "$OUT/build-platinum-$1.log"; exit 1; fi; }
# plat_case NAME FRAMES SCRIPT SAVE MAKEVARS... : relink, run, judge (the CHECKS are taken from PLAT_CHECKS).
plat_case(){ local name=$1 frames=$2 script=$3 save=$4; shift 4; want "$name" || return 0
  plat_build "$name" "$frames" "$script" "$@"
  (cd "$A" && python3 run_probe.py --rom "$PLAT_ROM" --ppsspp "$PPSSPP" --seconds 600 ${save:+--save "$save"}) > "$OUT/$name.probe.txt" 2>&1 || true
  keep_run "$name" "$A"; judge "$name" "$OUT/$name.log" "${PLAT_CHECKS[@]}"; }
plat_tests(){
  cp -f "$ROOT/port/native-audio-app/run_probe.py" "$A/"
  PLAT_CHECKS=("log:[AUDIO-SAS] ready: sceSasCore")
  plat_case boot 1800 "" ""   # blank save: title, intro and the first prompt; 60 s of game time
  [ "$QUICK" = 1 ] && return
  if [ -n "$PLAT_SAVE" ]; then
    # Oreburgh City south end into the Mine: the long conveyor belts have bounding boxes larger than the view,
    # which the box test used to cull at random. The screenshot at frame 2000 (about ten steps south) must still show them.
    PLAT_CHECKS=("log:[DIAG] warp done" "belts:2000")
    plat_case oreburgh-belts 2000 oreburgh-walk-south.h "$PLAT_SAVE" WARP_TO=45,302,775
    # Floaroma Town's south gate: its translucent arch and shade are submitted before the ground. Drawn in
    # submission order they came out as a solid black block (fixed by deferring translucent batches).
    PLAT_CHECKS=("log:[DIAG] warp done")
    plat_case floaroma-gate 2000 floaroma-sign.h "$PLAT_SAVE" WARP_TO=426,171,663
    # Oreburgh City into the Underground the way the Explorer Kit does. It used to crash twice over: the wireless icon
    # palette is opened by a path that differs in case from the ROM's file name, then the WM wireless library aborted
    # (no DS radio on the PSP). Must arrive with the WM stand-in running and stay up to the frame bound.
    PLAT_CHECKS=("log:[DIAG] entering the Underground" "log:[OVERLAY] load id=23" "log:[WIRELESS] WM stand-in")
    plat_case underground 5400 underground.h "$PLAT_SAVE" WARP_TO=45,302,775 UNDERGROUND=1
    # A double battle where every Pokemon is a Clefairy that knows only Sing. Sing's notes are polygon particles, whose
    # draw called the SDK port's MTX_Scale43_ stub and aborted, in any battle (fx_mtx_native.c is now linked).
    PLAT_CHECKS=("log:[DIAG] double battle vs trainer")
    plat_case sing-double 7000 double-sing.h "$PLAT_SAVE" DOUBLE_SING=1
  else result PASS oreburgh-belts floaroma-gate underground sing-double "skipped (needs --platinum-save, see tests/README.md)"; fi
}

### main #####################################################################################################
if [ "$LIST" = 1 ]; then cat <<'EOF'
platinum:   boot (quick) oreburgh-belts floaroma-gate underground sing-double (need --platinum-save)
soulsilver: smoke (quick) pc catch easychat pokedex apricorn vs-recorder trainer-card options options-confirm options-quit
            options-b options-scene options-nochange options-after geonet gym-pryce rocket-radio-tower
            qol-friendship-evo qol-trade-item-evo qol-trade-level-evo qol-repel-yes qol-repel-no
EOF
exit 0; fi
[ -n "$PLAT_ROM$SS_ROM" ] || die "pass --platinum-rom and/or --soulsilver-rom (see tests/README.md)"
if [ -n "$PPSSPP" ]; then [ -x "$PPSSPP" ] || die "PPSSPP_HEADLESS=$PPSSPP is not an executable"; PPSSPP="$(cd "$(dirname "$PPSSPP")" && pwd)/$(basename "$PPSSPP")"
else echo "PPSSPP_HEADLESS is not set: building and auditing only (see tests/README.md for the emulator scenarios)"; fi
for game in platinum soulsilver; do
  rom=$PLAT_ROM; [ $game = soulsilver ] && rom=$SS_ROM; [ -n "$rom" ] || continue
  if [ "$SKIP_BUILD" = 1 ]; then [ -d "$T" ] || die "--skip-build needs an existing .work tree"; result PASS "build-$game" "skipped (--skip-build)"
  elif "$ROOT/build.sh" $game --rom "$rom" > "$OUT/build-$game.log" 2>&1; then result PASS "build-$game" "EBOOT built, loader audit passed"
  else result FAIL "build-$game" "see .work/tests/build-$game.log"; tail -5 "$OUT/build-$game.log"; continue; fi
  [ -n "$PPSSPP" ] || continue
  log "Replaying $game scenarios in headless PPSSPP"
  if [ $game = platinum ]; then plat_tests; else ss_tests; fi
  # Put the shipping link back so dist/ and .work hold what ./build.sh produces (fast: every phase is stamped).
  "$ROOT/build.sh" $game --rom "$rom" > "$OUT/restore-$game.log" 2>&1 || result FAIL "restore-$game" "see .work/tests/restore-$game.log"
done
echo; echo "$PASSED passed, $FAILED failed (details: .work/tests/summary.txt, logs and screenshots in .work/tests/)"
[ "$FAILED" = 0 ]
