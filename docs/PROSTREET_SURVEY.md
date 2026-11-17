# NFS: ProStreet (Xbox 360): survey and first recompile

This is the work-in-progress record of bringing *Need for Speed: ProStreet*
(EA Black Box, 2007) to this tree as a third game, `SB_GAME=prostreet`. It
covers the static survey (phase 2) and the first clean recompile (phase 3).
Booting the game is the next round.

All numbers come from the author's own USA disc. No game data is in the
repository: `game-prostreet/` and `ppc-prostreet/` are gitignored.

## The disc

| | |
|---|---|
| Image | `Need for Speed - ProStreet (USA) (En,Es).iso`, 7.5 GB, 57 files, 5,495,112,692 bytes |
| `default.xex` | 4.6 MB, SHA-256 `4f8b623a2a1ed06555b57da4e70411574ac962bce4b81bb100629ecc518275c2` |
| Title ID | `0x45410822` (from the XEX; `runtime/install/manifest_prostreet.inc`) |
| Other modules | none: no `.xexp` or extra `.xex`. `$SystemUpdate/su20076000_00000000` is the dashboard update, not game code. |
| Movies | `Movies/*.vp6`: EA's VP6 is decoded by the game itself, not the XDK's XMV player as in MW and Carbon (`.wmv`). So the XMV movie-pacing hooks have nothing to attach to. |
| Data | `NFS/*`: `.BIN`/`.BUN`/`.LZC` archives, the same EA asset family as Carbon's |

## The image (`survey/xexsurvey.cpp`)

XenonRecomp's loader decrypts and decompresses the XEX with no changes. The
image is at base `0x82000000`, size `0x01140000`, entry point `0x82A679C8`.

| Section | Start | End | Size |
|---|---|---|---|
| .rdata | 0x82000400 | 0x820F1354 | 0xF0F54 |
| .pdata | 0x820F1400 | 0x821344C0 | 0x430C0 |
| .text | 0x82140000 | 0x82ADCED4 | 0x99CED4 (9.6 MB; Carbon's is 10.1 MB) |
| .data | 0x82AE0000 | 0x8309C790 | 0x5BC790 |
| .XBMOVIE, .tls, .idata, .XBLD, .reloc | | | small |

These ranges are in `survey/games.py` under `prostreet`.

## Compiler helpers: all found

The XDK CRT layout is the same as Most Wanted's and Carbon's: restgpr =
savegpr + 0x50, and vmx_64 = vmx_14 + 0x94.

| Helper | Address | How it was found |
|---|---|---|
| savegprlr_14 / restgprlr_14 | 0x82A280A0 / 0x82A280F0 | xexsurvey byte pattern, one hit |
| savefpr_14 / restfpr_14 | 0x82A28720 / 0x82A2876C | pattern |
| savevmx_14 / restvmx_14 | 0x82A2A2A0 / 0x82A2A538 | pattern |
| savevmx_64 / restvmx_64 | 0x82A2A334 / 0x82A2A5CC | pattern |
| longjmp | 0x82A26C50 | The only `.pdata` function that loads r14 to r31 (18 `ld`, from r7). It uses `mtcrf` and calls RtlUnwind (thunk 0x82ADBA24). |
| setjmp | 0x82A28DB0 | A leaf. It has Carbon's prologue: hook check (`lis r4,0x830A` / `lwz` / `bnectr`, hook pointer 0x8309C704), then `mflr`/`mfcr` and 19 `std` into r3. |

## Imports

There are 349 distinct imports: 154 from xam.xex and 180 functions plus 15
data imports from xboxkrnl.exe. The recompiled code references 334 of them.
The counts below come from `scripts/gen_import_stubs.py`.

| | Count |
|---|---|
| Implemented by the runtime already | 208 |
| Stubbed already (`kernel/import_stubs.cpp`, shared with MW) | 101 |
| Stubbed for ProStreet (`kernel/import_stubs_prostreet.cpp`) | 25 |
| Data imports | 15, all handled generically by `kernel/module.cpp` |

None of the runtime's data imports is new. These function imports are ones
that neither MW nor Carbon uses:

- `IoDismountVolume`, `IoDismountVolumeByFileHandle`
- `MmLockAndMapSegmentArray`, `MmUnlockAndUnmapSegmentArray`
- `NtDeviceIoControlFile`, `NtReadFileScatter`, `NtWriteFileGather`. Scatter/gather reads are likely on the streaming path, so they are likely needed early.
- `StfsControlDevice`, `StfsCreateDevice`: save-game devices
- `XAudioGetSpeakerConfig`
- `XamTaskSchedule`, `XamTaskCloseHandle`, `XamTaskShouldExit`: XAM worker tasks
- `XeCryptSha`, `XeKeysConsolePrivateKeySign`, `XeKeysConsoleSignatureVerification`: save signing

The full network stack (NetDll, XNet, QoS) is imported, as in Carbon. The
offline stack from Carbon (`kernel/imports_net.cpp`) covers it.

## Recompile (phase 3)

The pipeline is the order in `config/prostreet/prostreet.toml`'s header:
XenonAnalyse, then `fix_switch_tables.py`, recompile, `find_missing_tables.py`,
`fix_switch_tables.py`, `leaf_switch_bounds.py --write`, recompile, and
`find_entry_points.py --write`.

| Pass | Fatal problems |
|---|---|
| First pass: .pdata only, XenonAnalyse's tables | 4,619. Almost all were jump tables in leaf functions without bounds, plus 3 unknown opcodes. |
| With leaf bounds (166) and inline tables (39) | 61 |
| With 2 hand-added tables and 2 stub bounds | 30, all from the 3 unknown opcodes |
| With XenonRecomp patch 0010 | **0** |

The result:
- 84,148 functions and 789 switch tables (5 dropped as borrowed bounds checks, 2 added by hand).
- 6 secondary entry points; Carbon also has 6.
- 173 MB of C++ in `ppc-prostreet/`.

Hand fixes, all in `survey/games.py`, each the same kind Carbon needed:

- **Two `b +4` tail-call stubs.** Without an explicit size XenonRecomp treats the `b` as internal and swallows the next function along with its jump tables.
  - 0x823C37C8, 0x10 bytes
  - 0x824BBC90, 0x28 bytes
- **Two byte-offset jump tables** that XenonAnalyse misses. Each is `lbzx r0,table,rI; add r12,base,r0; bctr` with no bounds check.
  - 0x8286ECE4: table 0x82071B70, index r10, 9 cases
  - 0x82898F34: table 0x820719B0, index r3, 12 cases

**New PPC instructions** (XenonRecomp patch 0010, `patches/xenonrecomp/0010-*.patch`):

| Instruction | Sites | Implementation |
|---|---|---|
| `vcmpgtuw` / `vcmpgtuw.` | 8 | Unsigned word compare: xor the sign bit, then signed compare |
| `vpkuwus` | 12 | Clamp to 0xFFFF, then `packus_epi32` |
| `vslo` | 10 | Whole-vector octet shift as a `pshufb` on XenonRecomp's reversed vector layout |

These are tested by `tests/instr/prostreet_*.s` (41 checks against the
independent Python model in `tests/gen_instr_tests.py`). The full suite is 395
tests: Xenia's, MW's, Carbon's and ProStreet's. All pass.

## Build wiring (`SB_GAME=prostreet`)

- **Engine paths.** A ProStreet build defines `SB_GAME_PROSTREET` and also
  `SB_GAME_CARBON`, so it takes Carbon's engine paths: the smaller Auto
  resolution budget, no Most Wanted PAL tables, MW-only settings hidden, and
  no MW 1600-pitch shadow target. Carbon's HDR grade is keyed by a Carbon
  shader hash, so it stays inert.
- **Identity.** ProStreet has its own app name, folders, installer subfolder
  and manifest (`game_info.h`, `user/paths.h`, `install/placement.h`,
  `install/manifest.cpp`).
- **Left out of the build:**
  - every game's hooks (`runtime/game/*.cpp`, `runtime/game/carbon/`), since they patch other games' addresses;
  - `debug/d3d_waits.cpp`;
  - `apu/ea_cpu_budget.cpp`, which hooks MW's or Carbon's EA mixer by address.
- **Its own game folder.** `runtime/game/prostreet/` holds no-hook stubs of the
  engine's game interface (`game/*.h`), as Carbon's `hfr.cpp`/`language.cpp`
  do. Nothing there patches a guest address yet.

## Boot (round 2): it boots and races

The game runs on macOS arm64 from intro to a race at 50-60 fps, with sound
and a PS4 pad. That took four runtime changes, each found from a crash or
a hang report:

| Symptom | Cause | Fix |
|---|---|---|
| Abort opening `NFS\ZDIR.BIN` | `NtQueryInformationFile` class 27, `XFileXctdCompressionInformation` | Answered as Xenia does: `STATUS_INVALID_PARAMETER` with a zeroed buffer, so the file is treated as not XCTD-compressed. Class 26 (sector, a path hash) was added too. |
| Exit at boot | `XamUserCreateAchievementEnumerator` was a stub | An empty enumerator |
| Exit at boot | `XAudioGetSpeakerConfig` was a stub | XCONFIG's user audio flags, the same word `ExGetXConfigSetting` gives |
| Black screen after the intro (hang report: the main thread in `SwitchToThread` loops) | The game's `GetExitCodeThread` (XAPI, `sub_82A6C9B8`) reads the KTHREAD's `SignalState` (+4) and exit status (+0x140). Runtime handles are host objects, so the loader thread looked alive forever. | Guest threads record their exit code. ProStreet's `GetExitCodeThread` is replaced (`game/prostreet/xapi_threads.cpp`). |

Most Wanted and Carbon never call these. Carbon, rebuilt from this tree,
still boots and renders.

## Risks and next round

1. **Imports on the boot path.** Implement the new imports the boot actually
   hits, found with the kernel trace. Scatter/gather file I/O and the STFS
   device are the likely first ones.
2. **The 5 dropped tables.** These are the "borrowed bounds check" tables
   from `fix_switch_tables.py`. Check that they are not real switches.
   XenonRecomp raised no error for them.
3. **GPU.** ProStreet is a later, heavier renderer (2007). Expect new Xenos
   features in `shader_translator.cpp` and `renderer.cpp`. The method is
   `NFSMW_DUMP_SHADERS`, then `survey/xenos_survey.cpp`.
4. **Game hooks to find again.** Carbon's are the fingerprints, since the
   engine lineage is the same:
   - the frame-time lock
   - the D3D GPU-wait busy-spin (`game/carbon/d3d_waits.cpp`)
   - the render-queue barriers
   - the EA mixer budget
5. **Movies.** VP6, decoded in software by the game, so there is no XMV
   pacing to port. Its cadence may still need a look.
