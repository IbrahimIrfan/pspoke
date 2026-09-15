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
[ -x "$DEST/bin/psp-gcc" ] && [ "$(cat "$DEST/.pspoke-release" 2>/dev/null)" = "$RELEASE $ASSET" ] && { echo "    PSP toolchain $RELEASE already installed"; exit 0; }
need curl "Install curl."; need tar "Install tar."
log "Downloading the PSP toolchain ($RELEASE, about 150 MB, one time)"
mkdir -p "$CACHE"; TMP="$CACHE/$ASSET.part"
curl -fL --progress-bar -o "$TMP" "https://github.com/pspdev/pspdev/releases/download/$RELEASE/$ASSET" || die "download failed (check your internet connection and run the command again)"
GOT=$(if command -v sha256sum >/dev/null; then sha256sum "$TMP"; else shasum -a 256 "$TMP"; fi | cut -d' ' -f1)
[ "$GOT" = "$SHA" ] || { rm -f "$TMP"; die "toolchain download is corrupted (SHA-256 mismatch); run the command again"; }
rm -rf "$DEST" "$CACHE/pspdev.extract"; mkdir -p "$CACHE/pspdev.extract"
tar xzf "$TMP" -C "$CACHE/pspdev.extract" && mv "$CACHE/pspdev.extract/pspdev" "$DEST" && rm -rf "$CACHE/pspdev.extract" "$TMP"
echo "$RELEASE $ASSET" > "$DEST/.pspoke-release"
"$DEST/bin/psp-gcc" --version >/dev/null 2>&1 || die "the toolchain was downloaded but does not run on this computer"
echo "    PSP toolchain $RELEASE installed in .cache/pspdev"
