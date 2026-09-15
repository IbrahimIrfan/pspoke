#!/usr/bin/env bash
# stage.sh WORK : lay out the build tree in WORK (port sources + third-party + fetched upstream), fill in path placeholders.
source "$(dirname "$0")/common.sh"
WORK=$1; T="$WORK/test_out"
mkdir -p "$T" "$WORK/tools" "$WORK/melon"
rsync -a "$ROOT/port/" "$T/"
rsync -a --exclude README.md "$ROOT/third_party/melonDS/" "$WORK/melon/"
cp -f "$ROOT/scripts/check_native_pbp.py" "$WORK/tools/check_native_pbp.py"
{ grep -rlI -e '@WORK@' -e '@PSPDEV@' -e '@PPSSPP@' "$T" || true; } | while read -r f; do
  sed -i.bak -e "s#@WORK@#$T#g" -e "s#@PSPDEV@#$PSPDEV#g" -e "s#@PPSSPP@#${PPSSPP_HEADLESS:-PPSSPPHeadless}#g" "$f" && rm -f "$f.bak"
done
