# pspoke

Pokémon Platinum and SoulSilver running **natively on a PSP**. The game is compiled into a real PSP program
(it is not an emulator), so it runs at close to full speed on real hardware (tested on a PSP-3001).

> **Beta.** pspoke is still in beta: expect bugs, and keep backups of your saves. Bug reports and contributions are
> very welcome, see [Bug reports and contributing](#bug-reports-and-contributing).

> **Bring your own ROM.** This repository contains **no** game code, graphics, music, text, ROMs, saves or prebuilt
> EBOOTs. It is a set of build scripts, PSP platform code and patches. You build the EBOOT yourself, on your own
> computer, from the community decompilation projects and **your own legally dumped cartridge**.
>
> pspoke is an unaffiliated fan project, not associated with or endorsed by Nintendo, Game Freak, Creatures or
> The Pokémon Company. Please don't ask for, or post links to, ROMs or prebuilt builds in issues.

## Status

| Game | ROM the build accepts (SHA1) | State |
|---|---|---|
| Pokémon Platinum (US, Rev 1) | `0862ec35b24de5c7e2dcb88c9eea0873110d755c` | Playable. ~26-30 fps in the heaviest city on a PSP-3001, 30 fps elsewhere. |
| Pokémon SoulSilver (US) | `f8dc38ea20c17541a43b58c5e6d18c1732c7e582` | Playable. ~29-30 fps in towns on a PSP-3001. |

Sound works. Both games include a few optional quality-of-life changes (trade evolutions without trading, instant
text option, repel re-use prompt).

## Not supported yet

- **Wi-Fi and online features:** GTS, Wi-Fi Plaza, online trades and battles, Mystery Gift over the internet.
- **DS wireless:** local trades and battles with another DS, Union Room, DS Download Play.
- **Microphone** features.
- **Pokéwalker** (SoulSilver): the PSP has no infrared port.
- **Pal Park migration** (Platinum): there is no GBA cartridge slot.
- **Other versions and regions:** only the two US ROMs listed above build (no HeartGold, Diamond/Pearl or
  non-English releases).
- **Touch screen:** there is no real touchscreen, so touch input uses the on-screen cursor (see Controls).
- **Performance:** SoulSilver can dip below 30 fps in busy areas.
- **Tested hardware and computers:** only a PSP-3001 on ARK-4 has been tested. Building works on macOS and Linux;
  Windows (WSL) has not been tested.

## What you need

- A PSP running custom firmware that can launch homebrew (tested: PSP-3001 on ARK-4).
- Your own ROM dump of the game (the build checks the SHA1 above).
- A Mac or a Linux PC with about 3 GB of free disk space and an internet connection.

You don't need to install the PSP toolchain or any libraries yourself: the build downloads a pinned copy of the
PSP compiler ([PSPDEV](https://github.com/pspdev/pspdev), about 150 MB) into the project folder the first time.

## Getting started (step by step)

**1. Open a terminal.**
- Mac: press ⌘ Space, type `Terminal`, press Enter.
- Linux: open your Terminal app.

**2. Install the basic tools (one time).**
- Mac: run `xcode-select --install` and click **Install** in the window that appears (Apple's free command line
  tools; if you skip this step, the build opens the same window for you).
- Ubuntu/Debian: `sudo apt install git python3 make patch rsync curl build-essential`
- Fedora: `sudo dnf install git python3 make patch rsync curl gcc`

**3. Download pspoke.**

```sh
git clone https://github.com/IbrahimIrfan/pspoke.git
cd pspoke
```

**4. Build.** Type the command below, then drag your ROM file from Finder (or your file manager) into the terminal
window to fill in its path, and press Enter:

```sh
./build.sh platinum --rom 
```

For SoulSilver use `./build.sh soulsilver --rom ` the same way.

The first build downloads the toolchain and the decompilation sources (about 1 GB) and takes roughly 10-20 minutes.
Later builds are much faster. When it finishes you get `dist/platinum/NativePlatinum/EBOOT.PBP` (or
`dist/soulsilver/NativeSoulSilver/EBOOT.PBP`). You don't need a Platinum ROM to build SoulSilver.

Useful extras:
- Menu icon and background: put `ICON0.PNG` (144x80) and `PIC1.PNG` (480x272) in `art/platinum/` or
  `art/soulsilver/` before building (see [art/README.md](art/README.md)).
- `./build.sh setup` checks the requirements and downloads the toolchain without building anything.
- `./build.sh clean` removes the build output and starts over (downloads are kept).
- Already have your own PSPDEV install? Set `PSPDEV=/path/to/pspdev` and the build uses it instead.
- Windows: not tested. It may work inside WSL (Ubuntu) by following the Linux steps.

## Install on the PSP

Connect the memory stick (USB mode) and run:

```sh
scripts/install.sh platinum   "/Volumes/<your memory stick>" "/path/to/your/Pokemon Platinum.nds"
scripts/install.sh soulsilver "/Volumes/<your memory stick>" "/path/to/your/Pokemon SoulSilver.nds"
```

This creates `PSP/GAME/NativePlatinum/` (or `NativeSoulSilver/`) with the EBOOT, a copy of your ROM and, only if you
don't already have one there, a blank save. It never overwrites an existing save. To do it by hand:

```
PSP/GAME/NativePlatinum/EBOOT.PBP                <- dist/platinum/NativePlatinum/EBOOT.PBP
PSP/GAME/NativePlatinum/Platinum.nds             <- your ROM
PSP/GAME/NativePlatinum/Platinum.native.sav      <- python3 scripts/make_save.py Platinum.native.sav (new game)

PSP/GAME/NativeSoulSilver/EBOOT.PBP              <- dist/soulsilver/NativeSoulSilver/EBOOT.PBP
PSP/GAME/NativeSoulSilver/SoulSilver.nds         <- your ROM
PSP/GAME/NativeSoulSilver/SoulSilver.native.sav  <- python3 scripts/make_save.py SoulSilver.native.sav
```

Then launch it from the PSP's Game menu. Keep the CPU at the default speed; the game sets 333 MHz itself.

**Controls:** the PSP buttons map to the DS buttons (○ = A, ✕ = B, △ = X, □ = Y, L/R, START, SELECT, D-pad).
The touch screen is emulated with a cursor:

- **R** (or SELECT + ✕) toggles stylus mode.
- In stylus mode the **analog stick** moves the cursor (push further to move faster) and **L** taps the screen.
  The D-pad and the other buttons keep working normally, so you can mix touch and button input.
- Platinum only: SELECT + □ toggles sound.

**Saves:** `Platinum.native.sav` / `SoulSilver.native.sav` are normal 512 KB DS saves. You can bring over a save from a DS emulator or
cartridge dump by renaming it (it must be exactly 524,288 bytes). Back it up before experimenting.

## Developer builds

```sh
./build.sh platinum --rom "/path/to/Platinum.nds" --dev
```

`--dev` builds `dist/platinum-dev/` with an on-screen FPS/timing counter and detailed performance lines written to
`native-memlog.txt` next to the EBOOT (frame timing, 3D renderer profile, game-thread profile, GPU sync stats).
Normal builds have no on-screen counter and only log errors. See [docs/DEVELOPING.md](docs/DEVELOPING.md).

## Bug reports and contributing

pspoke is a beta, so reports and help are very welcome.

**Found a bug?** Open an issue at [github.com/IbrahimIrfan/pspoke/issues](https://github.com/IbrahimIrfan/pspoke/issues) with:
- the game and what happened (what you did right before, and whether it happens again),
- your PSP model and firmware, and your computer's OS if the build failed,
- the pspoke version (`git log -1 --oneline`),
- if you can, `native-memlog.txt` from the game's folder on the memory stick (a `--dev` build logs more detail).

Please never attach or link ROMs, saves from someone else's game, or prebuilt EBOOTs.

**Want to contribute?** Pull requests are welcome, especially for:
- **Windows support** (building natively or confirming WSL works),
- **other games:** Diamond/Pearl, HeartGold, and other regions or languages of Platinum/SoulSilver,
- anything under [Not supported yet](#not-supported-yet), and performance (SoulSilver's busy areas in particular),
- testing on other PSP models and firmware.

[docs/DEVELOPING.md](docs/DEVELOPING.md) explains how the build and the code are laid out. Keep the same rules as the
project: no ROM data, game assets or prebuilt EBOOTs in commits (changes to the decompilations go in `patches/`).

## How it works (short version)

1. `build.sh` checks your ROM and downloads the decompilation and NitroSDK-replacement sources at pinned commits
   (`third_party.lock`).
2. It applies pspoke's patches (`patches/`) and generates headers from the decompilation's data files.
3. It compiles the game's C code, the SDK libraries and pspoke's PSP platform layer (`port/`: memory/register
   emulation, threads, audio, file system, save, input, on-screen keyboard) for the PSP's MIPS CPU.
4. The DS 3D and 2D graphics commands are translated to the PSP GPU by pspoke's renderer (derived from melonDS).
5. Everything is linked into `EBOOT.PBP`, which is checked against the PSP firmware loader's limits.

At runtime the game reads its data files (graphics, maps, sound) from your ROM on the memory stick.

## Credits and licenses

- [pret](https://github.com/pret) and contributors: the Pokémon decompilation projects.
- [cybervisi0n/pokeplatinum](https://github.com/cybervisi0n/pokeplatinum), [libntr](https://github.com/cybervisi0n/libntr)
  (MIT), libntrsystem, libntrdwc, libntrwifi, libvct: Platinum port base and NitroSDK replacement.
- [antonsynd/pokeheartgold-slop](https://github.com/antonsynd/pokeheartgold-slop): HeartGold/SoulSilver decompilation fork.
- [melonDS](https://github.com/melonDS-emu/melonDS) (GPL-3.0): the DS 2D renderer pspoke's renderer is derived from.
- [pret/pokeheartgold](https://github.com/pret/pokeheartgold): used to check SoulSilver asset ordering.
- [ndspy](https://github.com/RoadrunnerWMC/ndspy) (GPL-3.0, bundled): reads the overlay table from your SoulSilver ROM.
- [metang](https://github.com/lhearachel/metang), [PSPDEV](https://github.com/pspdev).

pspoke is licensed under the **GNU General Public License v3.0** (`LICENSE`) because its renderer is derived from
melonDS. Third-party sources keep their own licenses and are downloaded at build time, not included here.
The games are the property of their respective owners.
