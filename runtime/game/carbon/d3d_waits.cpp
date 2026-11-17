// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: Carbon's Direct3D GPU waits (Most Wanted's: debug/d3d_waits.cpp).
// sub_826E0088 is D3D's GPU hang check, called once per poll by its
// busy-wait loops (9 call sites: fence, ring space, idle), with the same
// arguments as Most Wanted's: args[0] the device, args[8] the GPU counter as
// last seen, args[12] its time. The device keeps a pointer to the counter
// the GPU writes at +10768 and the last value issued at +10780 (Most
// Wanted's +10384 and +10396). Overridden here as in Most Wanted: back the
// waits off to short sleeps (cpu/gpu_wait_backoff.h, the one back-off of
// both games, Linux and macOS; without it the game's main thread spun a
// whole core in every 3D scene, and the Steam Machine's CPU sat at 85 C),
// park them while the game is suspended, and
// with NFSMW_LOG_D3D_WAIT=1 say once a second which counter they poll.
//
// sub_826DBBB0 is D3D's wait for a fence (r3 the device, r4 the fence
// value; Most Wanted's sub_82597498 takes a pointer to it): timed for the
// command processor's hitch reports ("game: ... D3D waits") and marked in
// NFSMW_TIMELINE, as debug/timeline.cpp does for Most Wanted. Carbon's
// Present isn't hooked (its D3D is a newer one), so a Carbon hitch report's
// "longest frame" and "present" stay 0.
#include <stdafx.h>
#include <kernel/function.h>
#include <cpu/guest_time.h>
#include <cpu/gpu_wait_backoff.h>
#include <debug/timeline.h>

// The game's addresses: Carbon's, unless a game with the same D3D code
// includes this file with its own (game/prostreet/d3d_waits.cpp).
#ifndef SB_D3D_HANG_CHECK
#define SB_D3D_HANG_CHECK sub_826E0088   // the GPU hang check
#define SB_D3D_FENCE_WAIT sub_826DBBB0   // the wait for a fence
#define SB_D3D_COUNTER_PTR 10768         // device + this: pointer to the counter the GPU writes
#define SB_D3D_ISSUED 10780              // device + this: the last value issued
#endif
#define SB_D3D_IMP_(name) __imp__##name
#define SB_D3D_IMP(name) SB_D3D_IMP_(name)

extern "C" PPC_FUNC(SB_D3D_IMP(SB_D3D_HANG_CHECK));
extern "C" PPC_FUNC(SB_D3D_IMP(SB_D3D_FENCE_WAIT));

PPC_FUNC(SB_D3D_FENCE_WAIT)
{
    timeline::Mark(timeline::GameWaitBegin, ctx.r4.u32);
    timeline::t_waitTargetMarked = false;
    const int64_t start = guesttime::NowNs();
    SB_D3D_IMP(SB_D3D_FENCE_WAIT)(ctx, base);
    timeline::g_gameWaitNs.fetch_add(uint64_t(guesttime::NowNs() - start), std::memory_order_relaxed);
    timeline::Mark(timeline::GameWaitEnd);
}

PPC_FUNC(SB_D3D_HANG_CHECK)
{
    guesttime::WaitWhileSuspended();
    static const bool log = std::getenv("NFSMW_LOG_D3D_WAIT") != nullptr;
    if (log)
    {
        static uint64_t polls = 0;
        static auto last = std::chrono::steady_clock::now();
        polls++;
        auto now = std::chrono::steady_clock::now();
        if (now - last > std::chrono::seconds(1))
        {
            uint32_t args = ctx.r3.u32;
            uint32_t device = PPC_LOAD_U32(args + 0);
            uint32_t counterPtr = PPC_LOAD_U32(device + SB_D3D_COUNTER_PTR);
            fprintf(stderr, "[d3dwait] %llu polls/s (lr %08X) | counter at %08X = %08X, issued %08X, last seen %08X\n",
                (unsigned long long)polls, uint32_t(ctx.lr), counterPtr, PPC_LOAD_U32(counterPtr),
                PPC_LOAD_U32(device + SB_D3D_ISSUED), PPC_LOAD_U32(args + 8));
            polls = 0;
            last = now;
        }
    }
    gpuwait::BackOff();
    SB_D3D_IMP(SB_D3D_HANG_CHECK)(ctx, base);
}
