# Changelog

What changed in each SpeedBreaker: ProStreet build, newest first. The game shows the entry once,
the first time a new version starts (offline: it's built into the game).

## v0.1.1

- Updated the port's runtime and code translation tools to the latest version for better stability. No visible
  changes in the game.

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
