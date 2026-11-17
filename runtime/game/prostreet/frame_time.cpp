// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING). See game/frame_time.h.
//
// NFS: ProStreet's game time in whole vblanks: Carbon's hook
// (game/carbon/frame_time.cpp, included below) on ProStreet's code, which
// times itself as Carbon does, with the same constants (65536.0, the
// 32000 ms cap, 1/3600 s ticks with a 1/4000 s remainder, 1/60 s steps,
// 59.999996, 1 to 8 steps). Its main loop (sub_825B6778) measures each frame
// with mftb (sub_82188410: ticks x 8e-5 ms) and hands it, in milliseconds as
// 16.16 fixed point, to the frame (sub_825B6368) at 0x825B68A8, Carbon's
// 0x824532A4 (midasm GameFrameTime). The frame's quantiser is sub_825CC498
// (Carbon's sub_824680C8; its call from the frame returns to 0x825B63B0),
// which keeps the frame's 1/3600 s ticks at 0x82BFE51C and its seconds at
// 0x8305D27C; the world's accumulator is sub_822A9E10 (Carbon's
// sub_82529BE0), with its step count at 0x822A9E98 (WorldStepCount, r11 and
// f0) and its below-one-step return at 0x822A9E64 (WorldNoStep).
//
// Where ProStreet differs (none of it changes the hooks): past 8 steps it
// returns the excess time instead of dropping it, which the frame ignores
// offline; a world with no pointer or dt <= 0 returns before the accumulator
// (so such frames aren't counted as no-step); and online (the byte at
// 0x82BFEA40, set by the online code only) the frame takes its world time
// from KeTimeStampBundle's tick count and may step the world more than once
// a frame. SpeedBreaker plays offline.
#define SB_FT_QUANTISER sub_825CC498
#define SB_FT_WORLD sub_822A9E10
#define SB_FT_QUANTISER_CALL 0x825B63B0
#define SB_FT_FRAME_TICKS 0x82BFE51C
#define SB_FT_FRAME_SECONDS 0x8305D27C
#include "../carbon/frame_time.cpp"
