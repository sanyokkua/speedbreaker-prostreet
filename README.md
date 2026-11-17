# SpeedBreaker: ProStreet

*Need for Speed: ProStreet* (EA Black Box, 2007) running natively on Apple Silicon Macs.

The Xbox 360 game's own code is translated to C++ and runs on **SpeedBreaker**'s runtime, the
engine project(u) built for [SpeedBreaker](https://github.com/SpeedBreakerProject/speedbreaker)
(*Most Wanted*) and [SpeedBreaker: Carbon](https://github.com/SpeedBreakerProject/speedbreaker-carbon).
This repository is a fork of SpeedBreaker: Carbon with ProStreet added as a third game
(`-DSB_GAME=prostreet`). Nearly everything here is their work: the recompiler setup, the
Xbox 360 kernel and GPU emulation, the Vulkan renderer, the installer, the settings menu. This fork
adds what ProStreet needed on top. Please support and credit the original projects.

**No game files are included.** You need your own **USA Xbox 360 disc** of NFS: ProStreet
(English and Spanish), as a disc image (ISO). The app installs the game from it.

How the port was made, and every fix ProStreet needed: [docs/PROSTREET_SURVEY.md](docs/PROSTREET_SURVEY.md).
What changed in each version: [CHANGELOG-prostreet.md](CHANGELOG-prostreet.md).
The original SpeedBreaker: Carbon README: [README-carbon.md](README-carbon.md).

## Play

1. Download `SpeedBreaker-ProStreet-macos.dmg` from [Releases](https://github.com/sanyokkua/speedbreaker-prostreet/releases/latest)
   (Apple Silicon Mac, macOS 15 or newer).
2. Open it and drag **SpeedBreaker ProStreet** to Applications.
3. **Open it.** The app isn't notarized by Apple, so the first time you open it, macOS refuses.
   Close that message (don't move the app to the Trash), then go to **System Settings > Privacy &
   Security**, scroll down to the message about SpeedBreaker ProStreet, click **Open Anyway** and
   confirm. Once per new download. On current macOS, Control-click > Open no longer skips this.
4. **Put your disc image in a `Games` folder in your home folder** (`~/Games`), where the app
   looks by itself, or pick it anywhere with *Browse for image...*.
5. **Install.** The app checks the image and installs the game (about 7.5 GB) into
   `~/Library/Application Support/SpeedBreaker ProStreet`, once.

Saves and settings live in that folder too. Settings: **F1** or **Cmd+,**, or Back + Start on a
controller.

### What you get

- 60 fps in races and menus, game time in whole display refreshes (no skipped or doubled world
  steps), and no CPU core spinning while the game waits for the GPU.
- Internal Resolution up to 3x (2160p), FSR 1 upscaling, HDR output, Brightness, Shadow
  Resolution, and Fill Screen for 16:10 and ultrawide screens.
- The menus in any of the 15 languages on the disc (speech and movies stay English).
- Controllers (Xbox, PlayStation and others SDL knows), keyboard, USB wheels (experimental).

On an M1 Pro, 2x Internal Resolution with Bicubic upscaling and 1x shadows holds 60 fps; FSR 1
and 2x shadows cost GPU time.

### Known issues

- Offline only. Two-player split screen hasn't been looked at.
- With Fill Screen on a 16:10 screen, some full-screen menu overlays don't reach the top and
  bottom edges.
- Movies and speech are English in every language (the disc has no others).

## Build it (macOS, Apple Silicon)

Needs Xcode's command line tools and, from Homebrew, `brew install llvm lld cmake ninja molten-vk`.

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

# The libraries the app bundles, from their official sources, for macOS 15 and newer.
scripts/macos/build_deps.sh build/deps

cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DSB_GAME=prostreet \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_PREFIX_PATH="$PWD/build/deps" -DFFMPEG_XMA="$PWD/build/deps/ffmpeg-xma"
ninja -C build/release

# The app and its dmg, in build/app-release.
python3 scripts/macos/make_app.py build/release build/app-release --dmg --min-macos=15.0
```

Run a build straight from the extracted files:

```sh
NFSMW_GAME_DIR="$PWD/game-prostreet/files" build/release/runtime/SpeedBreaker
```

Tests: `ctest --test-dir build/release` (`swapchain_use_test` can fail under a full parallel run
and passes alone) and `tests/run_instr_tests.sh` for the recompiler's instructions.

GitHub can't build releases: making the game's C++ needs the disc's `default.xex`, which is EA's
and never goes in this repository. Releases are built locally as above.

## ProStreet-specific notes

- **Memory ordering.** ProStreet's threads hand data to each other without barriers, relying on
  the Xbox's (and x86's) store order. ARM64 doesn't keep it, which showed as single wrong frames
  in busy scenes. A ProStreet build orders the translated code's memory accesses as x86 does
  (`SB_GUEST_TSO`, on by default for ProStreet).
- **Dynamic vertex buffers.** ProStreet refills large vertex buffers every frame; the renderer
  copies them at draw time instead of making the game wait for the GPU (a dropped frame about
  once a second before).
- **No HDR highlights.** Carbon's HDR output recovers light above white from its floating-point
  scene; ProStreet draws its scene in 8 bits, so HDR output here is the presenter's.
- **Movies** are EA VP6, decoded by the game; they keep their 16:9 shape under Fill Screen.
- The fixes in the shared renderer (a depth clear clearing overlapped depth targets, pixel
  shader depth export, dynamic vertex copies) are general and may help the other games too.

## Diagnostics

- `Cmd+D` (or `NFSMW_DRAW_LOG_AT=<seconds>`): log every draw of the next frame with its render
  state, to the session log in `~/Library/Application Support/SpeedBreaker ProStreet/logs`.
- `NFSMW_LOG_GUARD=<seconds>`, `NFSMW_LOG_D3D_WAIT=1`, `NFSMW_GAME_TIME_LOG=<file>`: what the game
  waits on, its GPU waits, and its frame times.
- Settings > Advanced > Save Bug Report: the log and system details on the Desktop.

## Credits

**SpeedBreaker and SpeedBreaker: Carbon are made by project(u)**
([SpeedBreakerProject](https://github.com/SpeedBreakerProject), [projectugarte.com](https://projectugarte.com/)).
This port only exists because of their engine and their documented approach. ProStreet's port
(this fork) by [sanyokkua](https://github.com/sanyokkua).

They in turn thank the **hedge-dev** team, for XenonRecomp and the approach Unleashed Recompiled
pioneered, and the **Xenia** project, for a decade of Xbox 360 research. The open-source projects
in this build:

| Project | What it's used for | License |
|---|---|---|
| [SpeedBreaker](https://github.com/SpeedBreakerProject/speedbreaker) and [SpeedBreaker: Carbon](https://github.com/SpeedBreakerProject/speedbreaker-carbon) (project(u) and contributors) | The whole runtime, renderer, installer and tools this fork builds on | GPL-3.0-or-later |
| [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) (hedge-dev and contributors) | The PowerPC-to-C++ recompiler; its XenonUtils (XEX loading) | MIT |
| [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp) (hedge-dev and contributors) | Parts of the kernel, XAM, memory and threading code | GPL-3.0-or-later |
| [Xenia](https://github.com/xenia-project/xenia) (Ben Vanik and Xenia contributors) | GPU register, microcode and Xenos headers; the reference for kernel, XMA and GPU behaviour | BSD-3-Clause |
| freedreno (Rob Clark) | Code inside the ported microcode header | MIT |
| [FFmpeg](https://ffmpeg.org/) 7.1.1 | XMA audio decoding (a decoder-only static build with a SpeedBreaker patch) | LGPL-2.1-or-later |
| [SDL3](https://www.libsdl.org/) | Windows, input, audio, app lifecycle | zlib |
| [Dear ImGui](https://github.com/ocornut/imgui) (Omar Cornut) | The installer and settings UI | MIT |
| [Roboto](https://fonts.google.com/specimen/Roboto) (Google) | The UI font | Apache-2.0 |
| [FidelityFX Super Resolution 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR) (AMD) | FSR 1 upscaling | MIT |
| [o1heap](https://github.com/pavel-kirienko/o1heap) (Pavel Kirienko) | The guest heap allocator | MIT |
| [glslang](https://github.com/KhronosGroup/glslang) and [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools) (Khronos) | Shader compilation at runtime | BSD-3-Clause and others; Apache-2.0 |
| [MoltenVK](https://github.com/KhronosGroup/MoltenVK) and the [Vulkan Loader](https://github.com/KhronosGroup/Vulkan-Loader) (Khronos) | Vulkan on a Mac | Apache-2.0 |
| [toml++](https://github.com/marzer/tomlplusplus) (Mark Gillard) | `settings.toml` | MIT |
| [SIMDe](https://github.com/simd-everywhere/simde) (Evan Nemerson and contributors) | SIMD in the recompiled code | MIT |
| [libmspack](https://github.com/kyz/libmspack) (Stuart Caie) | LZX decompression | LGPL-2.1 |
| [tiny-AES-c](https://github.com/kokke/tiny-AES-c) | XEX decryption | Unlicense |
| [TinySHA1](https://github.com/mohaps/TinySHA1) (Saurav Mohapatra) | SHA-1, via XenonUtils | ISC-style |
| [extract-xiso](https://github.com/XboxDev/extract-xiso) | Developer tool for unpacking disc images (not shipped) | Its own license |

Full license texts are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [LICENSES/](LICENSES/),
and in the app at `SpeedBreaker ProStreet.app/Contents/Resources/licenses`. The source code of
the LGPL parts in the app (FFmpeg with SpeedBreaker's patch, and libmspack) is published next to
each release, as `SpeedBreaker-ProStreet-<version>-third-party-source.tar.xz`.

## License

Free software under the **GNU General Public License**, version 3 or (at your option) any later
version (GPL-3.0-or-later), as SpeedBreaker. See [COPYING](COPYING) and [NOTICE](NOTICE).
Third-party components keep their own licenses, listed above.

Copyright (C) 2026 project(u) and SpeedBreaker contributors; ProStreet additions (C) 2026
sanyokkua.

## Legal notice

SpeedBreaker: ProStreet is an unofficial, fan-made project. It is **not affiliated with, endorsed
by or sponsored by Electronic Arts Inc., the Need for Speed brand, Microsoft or project(u).**

"Need for Speed" and "ProStreet" are trademarks of Electronic Arts Inc. Xbox and Xbox 360 are
trademarks of Microsoft. All other trademarks, including PlayStation, Apple, Mac, macOS, Vulkan
and AMD, belong to their respective owners and are used only to describe what this works with.
No logos, box art or key art of the game are used.

**No game files are included or distributed.** This project contains no game data or assets: no
textures, models, audio, movies or disc files. To play, you need your own legally obtained backup
(a disc image) of your own Xbox 360 disc of *Need for Speed: ProStreet* (2007), the USA release.
The in-app installer verifies it and installs it.

Provided "as is", without warranty of any kind, as set out in the GNU General Public License.
