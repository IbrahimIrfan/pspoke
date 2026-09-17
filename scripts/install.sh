#!/usr/bin/env bash
# install.sh GAME[-dev] MEMSTICK ROM : copy a built EBOOT to a PSP memory stick.
# Copies your ROM and creates a blank save only if they are not already there. Never overwrites a save.
source "$(dirname "$0")/common.sh"
GAME=${1:-}; MS=${2:-}; ROM=${3:-}
case "${GAME%-dev}" in platinum) DIR=NativePlatinum FILE=Platinum;; soulsilver) DIR=NativeSoulSilver FILE=SoulSilver;; *) die "usage: scripts/install.sh platinum|soulsilver[-dev] /Volumes/<memstick> <your.nds>";; esac
[ -d "$MS/PSP/GAME" ] || die "$MS does not look like a PSP memory stick (no PSP/GAME folder)"
SRC="$ROOT/dist/$GAME/$DIR/EBOOT.PBP"; [ -f "$SRC" ] || die "build first: ./build.sh ${GAME%-dev} --rom <file.nds>$([ "$GAME" != "${GAME%-dev}" ] && echo " --dev" || true)"
D="$MS/PSP/GAME/$DIR"; mkdir -p "$D"
cp -f "$SRC" "$D/EBOOT.PBP"
[ -f "${SRC%/EBOOT.PBP}/pspoke.cfg" ] && cp -f "${SRC%/EBOOT.PBP}/pspoke.cfg" "$D/pspoke.cfg"
[ -f "$D/$FILE.nds" ] || { [ -f "$ROM" ] || die "pass your ROM as the third argument"; cp "$ROM" "$D/$FILE.nds"; }
[ -f "$D/$FILE.native.sav" ] || python3 "$ROOT/scripts/make_save.py" "$D/$FILE.native.sav"
sync; log "Installed to $D"
