#!/bin/bash
# Rebuild ONE decompiled game object into both archives the native build links:
#   rebuild_game_object.sh src/battle/battle_input.c battle__battle_input.o 12
# (source path, archive member name, owning overlay id or "none").
# Mirrors what soulsilver-native-overlays/build_registry.py does for all objects.
set -e
export PATH=~/pspdev/bin:$PATH
C=@WORK@/soulsilver-native-core
OV=@WORK@/soulsilver-native-overlays
SRC="$1"; MEMBER="$2"; OWNER="$3"
cd "$C"
python3 - "$SRC" "$MEMBER" <<'PY'
import json,subprocess,sys
cmd=json.load(open('compile-command.json'))
r=subprocess.run(cmd+['-c',sys.argv[1],'-o','objects/'+sys.argv[2]],capture_output=True,text=True)
errs=[l for l in r.stderr.splitlines() if 'error' in l]
print('\n'.join(errs[:10]))
sys.exit(r.returncode)
PY
psp-ar r libsoulsilver-c.a "objects/$MEMBER"
/bin/cp -f "objects/$MEMBER" "$OV/game/$MEMBER"
if [ "$OWNER" != "none" ]; then
  ARGS=$(psp-objdump -h "$OV/game/$MEMBER" | awk '$2 ~ /^\.(bss|sbss)/{printf "--rename-section %s=.ssov.'"$OWNER"'.bss%s ",$2,$2} $2 ~ /^\.(data|sdata)/{printf "--rename-section %s=.ssov.'"$OWNER"'.data%s ",$2,$2}')
  [ -n "$ARGS" ] && psp-objcopy $ARGS "$OV/game/$MEMBER"
fi
psp-ar r "$OV/libss-game-overlays.a" "$OV/game/$MEMBER"
echo "rebuilt $MEMBER (overlay $OWNER) into libsoulsilver-c.a and libss-game-overlays.a"
