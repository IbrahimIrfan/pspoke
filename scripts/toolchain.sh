#!/usr/bin/env bash
# toolchain.sh : download the pinned prebuilt PSP toolchain (PSPDEV) for this computer into .cache/pspdev.
# Pinned to one release so every build uses the same compiler. Set PSPDEV yourself to use another install.
source "$(dirname "$0")/common.sh"
RELEASE=v20260901
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64)   ASSET=pspdev-macos-latest-arm64.tar.gz;     SHA=3d1308c94d437619569923d0b65a2d8cec6b6c7f3666d83959f9c38041a8e6f7;;
  Darwin-x86_64)  ASSET=pspdev-macos-15-intel-x86_64.tar.gz;  SHA=b6f5fff8593565e9ef56af6eb3ebed602c4c95cc0344a542726d2f4f45266fce;;
  Linux-x86_64)
    if grep -qi '^ID=fedora' /etc/os-release 2>/dev/null; then ASSET=pspdev-fedora-latest.tar.gz; SHA=136b0a4c0b880a99ce6dd886de2ecaeb58937e338d041dd4c6e45b4dab59326c
    elif grep -qi '^ID=debian' /etc/os-release 2>/dev/null; then ASSET=pspdev-debian-latest.tar.gz; SHA=90303df823b790b302868e8fd9930cba6f325a5904a662a05d6b4d8d0490004a
    else ASSET=pspdev-ubuntu-latest-x86_64.tar.gz; SHA=074eb72c36e491686987c045428e9a8bf39fd4e67cb4cd3ef36f4a3f01f54921; fi;;
  Linux-aarch64|Linux-arm64) ASSET=pspdev-ubuntu-24.04-arm-arm64.tar.gz; SHA=88e6352d58fb9b2424bab98fb67df54517663a5f25fb3ac288a16631fae86b12;;
  *) die "no prebuilt PSP toolchain for $(uname -s) $(uname -m). Install PSPDEV (https://pspdev.github.io) and set PSPDEV.";;
esac
DEST="$CACHE/pspdev"

# macOS: the prebuilt compilers load a few Homebrew libraries from fixed paths (/opt/homebrew on Apple Silicon,
# /usr/local on Intel; without them dyld stops with "Library not loaded: .../libisl.23.dylib" and similar) and are
# built for a minimum macOS (LC_BUILD_VERSION minos of this release: 26 arm64, 15 x86_64). Check before downloading.
mac_check(){
  [ "$(uname -s)" = Darwin ] || return 0
  local prefix min pkgs have p missing="" ans
  if [ "$(uname -m)" = arm64 ]; then prefix=/opt/homebrew; min=26; pkgs="gmp mpfr libmpc zstd"
  else prefix=/usr/local; min=15; pkgs="gmp mpfr libmpc isl zstd gettext"; fi
  have=$(sw_vers -productVersion | cut -d. -f1)
  [ "$have" -ge "$min" ] || die "the prebuilt PSP toolchain for this Mac needs macOS $min or newer (this is macOS $(sw_vers -productVersion)). Update macOS, or build PSPDEV yourself (https://pspdev.github.io) and set PSPDEV to it, or build in a Linux VM."
  for p in $pkgs; do [ -d "$prefix/opt/$p/lib" ] || missing="$missing $p"; done
  [ -z "$missing" ] && return 0
  [ -x "$prefix/bin/brew" ] || die "the PSP toolchain needs Homebrew installed in $prefix (https://brew.sh), then:  brew install$missing"
  echo "The PSP toolchain needs these Homebrew libraries:$missing"
  echo "To install them: brew install$missing"
  if [ "${PSPPOKE_ASSUME_YES:-0}" = 1 ]; then ans=y
  elif [ -t 0 ]; then printf 'Install them now? [y/N] '; read -r ans
  else die "not an interactive terminal: run the command above, then run the build again (or set PSPPOKE_ASSUME_YES=1)"; fi
  case "$ans" in y|Y|yes|YES) ;; *) die "run the command above, then run the build again";; esac
  # shellcheck disable=SC2086  # intentional word splitting: the package list printed above
  "$prefix/bin/brew" install $missing || die "brew install failed. Run the command above yourself, then run the build again."
}
# Compile an empty file: that runs the driver, cc1 and the assembler, which is where the libraries are loaded.
toolchain_runs(){
  local out
  out=$(echo 'int pspoke;' | "$DEST/bin/psp-gcc" -x c -c -o /dev/null - 2>&1) && return 0
  case "$out" in
    *"Library not loaded"*) die "the PSP toolchain cannot load a library it needs: $(echo "$out" | grep -m1 'Library not loaded'). On macOS run:  brew install gmp mpfr libmpc isl zstd gettext   then run the build again.";;
    *) die "the PSP toolchain was downloaded but does not run on this computer: $(echo "$out" | head -3)";;
  esac
}

mac_check
if [ -x "$DEST/bin/psp-gcc" ] && [ "$(cat "$DEST/.pspoke-release" 2>/dev/null)" = "$RELEASE $ASSET" ]; then
  toolchain_runs; echo "    PSP toolchain $RELEASE already installed"; exit 0
fi
need curl "Install curl."; need tar "Install tar."
log "Downloading the PSP toolchain ($RELEASE, about 150 MB, one time)"
mkdir -p "$CACHE"; TMP="$CACHE/$ASSET.part"
curl -fL --progress-bar -o "$TMP" "https://github.com/pspdev/pspdev/releases/download/$RELEASE/$ASSET" || die "download failed (check your internet connection and run the command again)"
GOT=$(if command -v sha256sum >/dev/null; then sha256sum "$TMP"; else shasum -a 256 "$TMP"; fi | cut -d' ' -f1)
[ "$GOT" = "$SHA" ] || { rm -f "$TMP"; die "toolchain download is corrupted (SHA-256 mismatch); run the command again"; }
rm -rf "$DEST" "$CACHE/pspdev.extract"; mkdir -p "$CACHE/pspdev.extract"
tar xzf "$TMP" -C "$CACHE/pspdev.extract" && mv "$CACHE/pspdev.extract/pspdev" "$DEST" && rm -rf "$CACHE/pspdev.extract" "$TMP"
echo "$RELEASE $ASSET" > "$DEST/.pspoke-release"
toolchain_runs
echo "    PSP toolchain $RELEASE installed in .cache/pspdev"
