# pspoke

![Status: beta](https://img.shields.io/badge/status-beta-yellow) ![Platform: PSP](https://img.shields.io/badge/platform-PSP-blue) [![Release](https://img.shields.io/github/v/release/IbrahimIrfan/pspoke?include_prereleases&label=release)](https://github.com/IbrahimIrfan/pspoke/releases) [![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-green)](LICENSE)

Pokémon Platinum and SoulSilver as native PSP programs. Not an emulator: the game is compiled for the PSP and runs
at close to full speed on real hardware.

This repository contains no game code, assets, ROMs, saves or prebuilt EBOOTs. It is build scripts, PSP platform
code and patches. You build the EBOOT yourself from the community decompilations and your own cartridge dump.
Not affiliated with or endorsed by Nintendo, Game Freak, Creatures or The Pokémon Company. No warranty; see
[LICENSE](LICENSE) (GPL-3.0, sections 15 and 16). Please don't ask for, or share, ROMs or prebuilt EBOOTs in the issues.

The code, patches, tests and documentation here were written by AI.

This is still a beta, so expect some rough edges, and keep a backup of your saves just in case.

## Status

| Game | ROM (SHA1) | State |
|---|---|---|
| Platinum (US, Rev 1) | `0862ec35b24de5c7e2dcb88c9eea0873110d755c` | Playable. 30 fps; 26-30 in the heaviest city. |
| SoulSilver (US) | `f8dc38ea20c17541a43b58c5e6d18c1732c7e582` | Playable. 29-30 fps in towns, lower in busy areas. |

Sound works. Tested on a PSP-3001 running ARK-4; builds on macOS and Linux.

### Not supported

- Wi-Fi and online features (GTS, Wi-Fi Plaza, online trades and battles, Mystery Gift over the internet).
- DS-to-DS wireless: local trades and battles, Union Room, Download Play.
- Microphone, Pokéwalker, Pal Park.
- Other regions or languages, and other games (HeartGold, Diamond/Pearl).
- A real touch screen: touch input uses a cursor instead (see Controls).
- PSP-1000 (32 MB). It won't run there, and that isn't planned: the port needs about 38 MB of RAM, and what's left is
  the DS memory map the game addresses directly rather than tunable buffers
  ([issue #10](https://github.com/IbrahimIrfan/pspoke/issues/10)). It does run on the PSP-2000/3000/Go/E1000 and
  on Vita/PSTV under Adrenaline.

## Building

You need:

- a PSP with custom firmware,
- your own ROM dump (checked against the SHA1s above),
- a Mac or Linux machine with about 3 GB free and an internet connection.

The build downloads a pinned PSP toolchain into the project folder the first time (~150 MB), so the only things to
install by hand are the basics:

- macOS: `xcode-select --install`
- Ubuntu/Debian: `sudo apt install git python3 make patch rsync curl build-essential`
- Fedora: `sudo dnf install git python3 make patch rsync curl gcc`
- Windows: use WSL2 (Ubuntu), clone inside the Linux filesystem (not `/mnt/c`), and follow the Linux steps. Toolchain
  setup is confirmed on WSL2; the game builds themselves are untested there.

Then:

```sh
git clone https://github.com/IbrahimIrfan/pspoke.git
cd pspoke
./build.sh platinum   --rom "/path/to/Platinum.nds"
./build.sh soulsilver --rom "/path/to/SoulSilver.nds"
```

The first build downloads the toolchain and decompilation sources (~1 GB) and takes 10-20 minutes; later builds are
fast. Output is `dist/platinum/NativePlatinum/EBOOT.PBP` or `dist/soulsilver/NativeSoulSilver/EBOOT.PBP`.

Other options:

- `./build.sh setup` downloads the toolchain without building anything.
- `./build.sh clean` starts over (downloads are kept).
- `PSPDEV=/path/to/pspdev` uses your own toolchain instead of the downloaded one.
- `--dev` builds a version with an on-screen fps counter and performance logging to `native-memlog.txt`
  ([docs/DEVELOPING.md](docs/DEVELOPING.md)).
- Menu icon and background: put `ICON0.PNG` (144x80) and `PIC1.PNG` (480x272) in `art/platinum/` or
  `art/soulsilver/` before building ([art/README.md](art/README.md)).

## Quality-of-life changes

Both games get a few changes, all on by default and each switchable off at build time. They only change what runs
in RAM; the ROM and save format are untouched, so saves move between builds with different flags.

| Change | Off with |
|---|---|
| Instant text | `--no-instant-text` |
| Trade evolutions without trading (level 36 or the held item from the Bag; happiness evolutions at any hour) | `--no-trade-evos` |
| "Use another?" prompt when a Repel wears off | `--no-repel-prompt` |
| HM moves can be forgotten | `--no-forget-hms` |
| Cut, Rock Smash and Whirlpool buffed | `--no-move-buffs` |

The full list, per Pokémon and item, is in [docs/QOL.md](docs/QOL.md).

## Installing on the PSP

With the memory stick connected in USB mode:

```sh
scripts/install.sh platinum   "/Volumes/<memory stick>" "/path/to/Platinum.nds"
scripts/install.sh soulsilver "/Volumes/<memory stick>" "/path/to/SoulSilver.nds"
```

This creates `PSP/GAME/NativePlatinum/` (or `NativeSoulSilver/`) with the EBOOT, a copy of your ROM, and a blank
save if none exists. It never overwrites a save. By hand, the layout is:

```
PSP/GAME/NativePlatinum/EBOOT.PBP               dist/platinum/NativePlatinum/EBOOT.PBP
PSP/GAME/NativePlatinum/Platinum.nds            your ROM
PSP/GAME/NativePlatinum/Platinum.native.sav     python3 scripts/make_save.py Platinum.native.sav
```

(SoulSilver: `NativeSoulSilver/`, `SoulSilver.nds`, `SoulSilver.native.sav`.) Launch it from the Game menu, and leave
the CPU clock at default; the game sets 333 MHz itself.

Saves are plain 512 KB DS saves (exactly 524,288 bytes). A save from an emulator or cartridge dump works if you rename it.

### Controls

- PSP buttons map to the DS: ○ A, ✕ B, △ X, □ Y, L/R, START, SELECT, D-pad.
- R (or SELECT+✕) toggles stylus mode. In stylus mode the analog stick moves the cursor and L taps; the D-pad and
  buttons keep working.
- Platinum only: SELECT+□ toggles sound.

## Bugs and contributing

If you hit a bug, open an issue with:

- the game, what happened, and whether it repeats,
- your PSP model and firmware (or your OS, if the build failed),
- the pspoke version (`git log -1 --oneline`),
- if possible, `native-memlog.txt` from the game's folder on the memory stick.

Please don't attach ROMs, other people's saves, or EBOOTs.

Pull requests are welcome; run `tests/run.sh` first ([tests/README.md](tests/README.md)). Most wanted:

- Windows builds,
- other games and regions,
- SoulSilver performance in busy areas,
- testing on other PSP models.

[docs/DEVELOPING.md](docs/DEVELOPING.md) explains the layout, and [NOTES.md](NOTES.md) collects what we learned the
hard way (read it before debugging on a real PSP). Same rules as the project: no game data in commits, and changes
to the decompilations go in `patches/`.

## How it works

1. `build.sh` checks your ROM and downloads the decompilation and NitroSDK-replacement sources at pinned commits
   (`third_party.lock`).
2. It applies the patches in `patches/` and generates headers from the decompilation's data.
3. It compiles the game, the SDK libraries and pspoke's PSP platform layer (`port/`: memory and register emulation,
   threads, audio, file system, save, input, on-screen keyboard) for the PSP's MIPS CPU.
4. DS 2D and 3D drawing is translated to the PSP GPU by a renderer derived from melonDS.
5. Everything is linked into `EBOOT.PBP` and checked against the firmware loader's limits.

At runtime the game reads its assets from your ROM on the memory stick.

## Credits and licenses

- [pret](https://github.com/pret) and contributors: the decompilation projects.
- [cybervisi0n/pokeplatinum](https://github.com/cybervisi0n/pokeplatinum), [libntr](https://github.com/cybervisi0n/libntr)
  (MIT), libntrsystem, libntrdwc, libntrwifi, libvct: Platinum port base and NitroSDK replacement.
- [antonsynd/pokeheartgold-slop](https://github.com/antonsynd/pokeheartgold-slop): HeartGold/SoulSilver decompilation fork.
- [melonDS](https://github.com/melonDS-emu/melonDS) (GPL-3.0): the DS 2D renderer ours is derived from.
- [pret/pokeheartgold](https://github.com/pret/pokeheartgold): SoulSilver asset ordering.
- [ndspy](https://github.com/RoadrunnerWMC/ndspy) (GPL-3.0, bundled): reads the overlay table from the SoulSilver ROM.
- [metang](https://github.com/lhearachel/metang), [PSPDEV](https://github.com/pspdev).

pspoke is GPL-3.0 (`LICENSE`) because the renderer is derived from melonDS. Third-party sources keep their own
licenses and are downloaded at build time. The games belong to their owners.
