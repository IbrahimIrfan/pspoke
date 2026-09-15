#!/usr/bin/env bash
# pspoke build entry point. See README.md.
#   ./build.sh platinum   --rom "path/to/Platinum.nds"   [--dev]
#   ./build.sh soulsilver --rom "path/to/SoulSilver.nds" [--dev]
#   ./build.sh setup     (optional: check requirements and download the PSP toolchain now)
#   Quality-of-life switches (all on by default): --no-instant-text --no-trade-evos --no-repel-prompt
#   ./build.sh clean
source "$(dirname "$0")/scripts/common.sh"
GAME="${1:-}"; shift || true
ROM=""; DEV=0
while [ $# -gt 0 ]; do case "$1" in
  --rom) ROM="$2"; shift 2;;
  --dev) DEV=1; shift;;
  --no-instant-text) export PSPOKE_QOL_INSTANT_TEXT=0; shift;;
  --no-trade-evos) export PSPOKE_QOL_TRADE_EVOS=0; shift;;
  --no-repel-prompt) export PSPOKE_QOL_REPEL_PROMPT=0; shift;;
  *) die "unknown option $1";;
esac; done

case "$GAME" in
  setup) ;;
  clean) rm -rf "$ROOT/.work" "$ROOT/dist"; log "Removed .work and dist (downloads in .cache kept)"; exit 0;;
  platinum)   EXPECT=0862ec35b24de5c7e2dcb88c9eea0873110d755c; NAME="Pokémon Platinum (US, Rev 1)";;
  soulsilver) EXPECT=f8dc38ea20c17541a43b58c5e6d18c1732c7e582; NAME="Pokémon SoulSilver (US)";;
  *) die "usage: ./build.sh platinum|soulsilver --rom <file.nds> [--dev] [--no-instant-text] [--no-trade-evos] [--no-repel-prompt]   (or ./build.sh setup, ./build.sh clean)";;
esac

if [ "$(uname -s)" = Darwin ] && ! xcode-select -p >/dev/null 2>&1; then
  xcode-select --install >/dev/null 2>&1 || true
  die "macOS needs Apple's free command line tools. An installer window should have opened: finish it, then run this command again."
fi
need git "Install git."; need python3 "Install Python 3.9+."; need make "Install make (Xcode command line tools / build-essential)."
need patch "Install patch."; need rsync "Install rsync."
if [ "$PSPPOKE_OWN_TOOLCHAIN" = 1 ]; then
  [ -x "$PSPDEV/bin/psp-gcc" ] || die "PSP toolchain not found at \$PSPDEV=$PSPDEV (unset PSPDEV to use the automatic download)."
else
  bash "$ROOT/scripts/toolchain.sh"
fi
[ "$GAME" = setup ] && { log "Setup complete. Next: ./build.sh platinum --rom <file.nds>  or  ./build.sh soulsilver --rom <file.nds>"; exit 0; }
[ -n "$ROM" ] || die "pass your own ROM with --rom <file.nds>"
[ -f "$ROM" ] || die "ROM not found: $ROM"
ROM="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"
log "Checking ROM"; GOT=$(sha1 "$ROM")
[ "$GOT" = "$EXPECT" ] || die "ROM SHA1 is $GOT; $NAME must be $EXPECT (use a clean, unmodified dump)"
exec "$ROOT/scripts/$GAME.sh" "$ROM" "$DEV"
