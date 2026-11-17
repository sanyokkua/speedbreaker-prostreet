# SpeedBreaker: ProStreet

*Need for Speed: ProStreet* (EA Black Box, 2007) running natively on Apple Silicon Macs: the
Xbox 360 game's code translated to C++ (XenonRecomp) and run on SpeedBreaker's runtime, the same
one as SpeedBreaker for Most Wanted and for Carbon. This tree is SpeedBreaker: Carbon's, with
ProStreet added as a third game (`-DSB_GAME=prostreet`).

No game data is included. You need your own **USA Xbox 360 disc** of NFS: ProStreet
(English and Spanish), as a disc image.

How it was made, and every fix ProStreet needed: [docs/PROSTREET_SURVEY.md](docs/PROSTREET_SURVEY.md).
What changed in each build: [CHANGELOG-prostreet.md](CHANGELOG-prostreet.md).

## What you get

- 60 fps in races and menus, game time in whole display refreshes (no skipped or doubled world
  steps), and no busy-waiting CPU core.
- Settings (F1 or Cmd+,, or Back + Start on a controller): Internal Resolution up to 3x (2160p), FSR 1
  upscaling, HDR output, Brightness, Shadow Resolution, Fill Screen for 16:10 and ultrawide
  screens, the language of the menus (15 on the disc), controls and wheel.
- Controllers (Xbox, PlayStation and others SDL knows), keyboard, USB wheels (experimental).
- Saves in `~/Library/Application Support/SpeedBreaker ProStreet/save`.

Offline only. Two-player split screen hasn't been looked at.

## Build it (macOS, Apple Silicon)

Needs Xcode's command line tools and, from Homebrew, `brew install llvm lld cmake ninja molten-vk`,
plus the libraries the game links: either Homebrew's (`brew install sdl3 vulkan-loader glslang
spirv-tools`, this Mac's macOS only, as below) or `scripts/macos/build_deps.sh` (from source, for
macOS 15 and newer; see the Carbon README's build section).

```sh
scripts/setup_tools.sh    # XenonRecomp + SpeedBreaker's patches, extract-xiso, FFmpeg's XMA decoder

# Extract your disc image into game-prostreet/files and check the executable.
mkdir -p game-prostreet/files
tools/extract-xiso/build/extract-xiso -d game-prostreet/files -x "/path/to/Need for Speed - ProStreet (USA) (En,Es).iso"
shasum -a 256 game-prostreet/files/default.xex
# must print 4f8b623a2a1ed06555b57da4e70411574ac962bce4b81bb100629ecc518275c2

# Translate the game's code into C++ (writes ppc-prostreet/, about 170 MB).
mkdir -p ppc-prostreet
tools/XenonRecomp/build/XenonRecomp/XenonRecomp config/prostreet/prostreet.toml tools/XenonRecomp/XenonUtils/ppc_context.h

# Build (for this Mac's macOS; the release scripts' scripts/macos/build_deps.sh
# builds the libraries for older macOS versions instead).
cmake -S . -B build/prostreet -G Ninja -DCMAKE_BUILD_TYPE=Release -DSB_GAME=prostreet \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DFFMPEG_XMA="$PWD/tools/ffmpeg-xma" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$(sw_vers -productVersion | cut -d. -f1-2)"
ninja -C build/prostreet

# The app: build/app-prostreet/SpeedBreaker ProStreet.app
python3 scripts/macos/make_app.py build/prostreet build/app-prostreet
```

Run it straight from the build with the extracted files:

```sh
NFSMW_GAME_DIR="$PWD/game-prostreet/files" build/prostreet/runtime/SpeedBreaker
```

The app instead asks for the disc image (or a folder of its files) at its first start and
installs the game into `~/Library/Application Support/SpeedBreaker ProStreet`.

Tests: `ctest --test-dir build/prostreet` (`swapchain_use_test` can fail under a full parallel
run; it passes alone) and `tests/run_instr_tests.sh` for the recompiler's instructions.

## ProStreet-specific notes

- **Memory ordering.** ProStreet's threads hand data to each other without barriers, relying on
  the Xbox's (and x86's) store order. ARM64 doesn't keep it, which showed as single wrong frames
  in busy scenes. A ProStreet build orders the translated code's memory accesses as x86 does
  (`SB_GUEST_TSO`, on by default for ProStreet).
- **No HDR highlights.** Carbon's HDR output recovers light above white from its floating-point
  scene; ProStreet draws its scene in 8 bits, so HDR output here is the presenter's (brighter
  whites and a wider range), without extra highlights.
- **Movies** are EA VP6, decoded by the game; they keep their 16:9 shape under Fill Screen.
- **Speech and movies are English** in every language: the disc has no other speech bank.

## Diagnostics

- `Cmd+D` (or `NFSMW_DRAW_LOG_AT=<seconds>`): log every draw of the next frame with its render
  state, to the session log in `~/Library/Application Support/SpeedBreaker ProStreet/logs`.
- `NFSMW_LOG_D3D_WAIT=1`, `NFSMW_GAME_TIME_LOG=<file>`: the GPU waits and the game's frame times.
- Settings > Advanced > Save Bug Report: the log and system details on the Desktop.

## License

GPL-3.0-or-later, as SpeedBreaker. Need for Speed and Need for Speed ProStreet are trademarks of
Electronic Arts; this project is not affiliated with or endorsed by EA or Microsoft.
