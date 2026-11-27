# Changelog

What changed in each SpeedBreaker: ProStreet build, newest first. The game shows the entry once,
the first time a new version starts (offline: it's built into the game).

## v0.1.1

### Fixed

- Fixes found while porting the Harry Potter Xbox 360 games, which run on the same runtime. None
  was seen to change ProStreet; each makes it behave more like the console:
  - Threads waiting for the same signal get it in the order they started waiting, as on the
    console (Half-Blood Prince froze at its first logo without this).
  - Rectangles the game draws to clear or copy the screen are filled whichever corner it gives
    first, and a clear reaches only the video memory the console's would.
  - Objects the game marks for only part of the screen are drawn (they blinked or vanished in
    Half-Blood Prince). ProStreet marks its objects for the whole screen.
  - The game's code is translated more exactly: 27 branches that were left out of the translated
    code, in the console's system library (exception handling and one list helper), are now in.
  - Clearing the screen now also clears the same video memory seen in another layout.
  - The memory guard no longer lets the game rewrite vertex data that the GPU has yet to read
    (one-frame "exploding" models in Order of the Phoenix).
  - A system-settings answer fixed (video region and country); ProStreet doesn't ask for those.

### For testers

A/B switches (environment variables, 0 = the old behaviour): `NFSMW_DRAW_CLEAR`,
`NFSMW_GUARD_NEIGHBOURS`, `NFSMW_TILE_PREDICATE`. Diagnostics: `NFSMW_GLITCH_WATCH=1`,
`NFSMW_GUARD_RACE=1`, `NFSMW_LOG_PREDICATE=1`.

## v0.1.0

### What's new

- **NFS: ProStreet runs natively on Apple Silicon Macs**, from your own USA Xbox 360 disc
  (English and Spanish), at 60 fps, with sound, controllers and saves.
- **Smoother driving**: game time moves in whole display refreshes, as in SpeedBreaker for Most
  Wanted and Carbon, so the world never skips or doubles a step.
- **Lower CPU use**: the game no longer spins a CPU core while it waits for the GPU.
- **Fill Screen** (Settings > Display > Aspect Ratio): on a screen of another shape than 16:9,
  such as a MacBook's 16:10, the race fills it with a taller view instead of black bars. Movies
  keep their 16:9 shape.
- **Language** (Settings > Game > Language): the menus and race text in any of the 15 languages
  on the disc. Movies and speech stay in English, as the disc has them.
- **Shadow Resolution** (Settings > Graphics): sharper sun shadows, 1.5x or 2x the 360's.
- Internal Resolution, upscaling, HDR output, Brightness and the rest of the Settings menu, as
  in SpeedBreaker for Carbon.

### Fixed on the way

- Tire smoke and dust next to your car are drawn, and hidden behind the car as they should be.
- Opponents no longer show through walls and hills.
- No more single wrong frames ("blinks") in busy scenes with crowds and many cars.
- No more dropped frame about once a second in races: the game no longer waits for the GPU
  each time it refills its dynamic vertex buffers.

### Not there yet

- Online play (offline only, as in the other SpeedBreaker games).
- Two-player split screen hasn't been looked at.
