// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING). See game/ultrawide.h.
//
// NFS: ProStreet's Hor+ and Vert+, Carbon's design (game/carbon/ultrawide.cpp)
// on ProStreet's code, which is Carbon's nearly instruction for instruction.
// Two mid-asm hooks (config/prostreet/prostreet.toml):
//   - UltrawideProjection, before `stfs f5,0(r29)` at 0x821DECCC in the view
//     setup sub_821DE9A8 (Carbon's sub_822CEA60): f5 = P[0][0], stored at
//     0x821DECCC and 0x821DED24 (view + 64 and + 128), f11 negated at
//     0x821DECE0 and stored as P[1][1] at 0x821DECE4 and 0x821DED28. r7 is
//     the view, its id at +8; the game's own 16:9 widening (0x821DEB04) tests
//     1, 5, 2, 6 and 40, the only views changed here (Carbon's 1, 4, 2, 5, 21).
//     The view table is at 0x82C099E0, 544 bytes an id, the target at +528.
//   - UltrawideFrontEnd, at the branch at 0x82284124 that ends the widescreen
//     case of the front-end matrix (sub_82284058): f13 its x scale, f0 its y
//     scale. ProStreet copies that matrix to 0x83041680 (sub_822661A0) and its
//     queued HUD draws upload it through sub_8226AF50 (Carbon's sub_824F4E98),
//     which caches its last view, matrix and object at 0x82BFD5C4, +4, +8: on
//     a change the cached matrix pointer is cleared so the next draw uploads it.
// And one function override: sub_82276488, the mirror's and picture-in-
// picture's rectangle builder (Carbon's sub_82505248; called by sub_822765E8
// and sub_82276820), narrowed about the screen centre as Carbon's.
//
// Movies: ProStreet's are EA VP6, drawn by the game with no 3D view and no
// XMV display aspect to patch. So Fill Screen is on only while the game draws
// a 3D view: HooksActive() is true while UltrawideProjection ran in the last
// half second, and the front-end matrix is only narrowed then. A movie or a
// screen without a 3D view is shown at 16:9 (pillarboxed or letterboxed), as
// it was made, instead of stretched.
#include <stdafx.h>
#include <game/ultrawide.h>
#include <game/front_end_matrix.h>

#include <kernel/function.h>

extern "C" PPC_FUNC(__imp__sub_82276488);

namespace
{
    // Per-view table: 0x82C099E0 + id * 544, the view's target at +528,
    // its width and height at +16 and +20.
    constexpr uint32_t kViewTable = 0x82C099E0, kViewStride = 544, kViewTarget = 528;

    struct Scales
    {
        float x, y;
    };
    std::atomic<Scales> s_scales{ Scales{ 1.0f, 1.0f } };
    // When UltrawideProjection last ran (steady clock, ns; 0: never).
    std::atomic<int64_t> s_lastProjectionNs{ 0 };
    constexpr int64_t kActiveNs = 500'000'000;

    int64_t NowNs()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // A 3D view drawn in the last half second (see the file's header).
    bool Active()
    {
        const int64_t last = s_lastProjectionNs.load(std::memory_order_relaxed);
        return last && NowNs() - last < kActiveNs;
    }
    std::atomic<float> s_frontEndX{ 0.0f };
    std::atomic<float> s_frontEndY{ 0.0f };

    bool WidenedViewId(uint32_t id)
    {
        return id == 1 || id == 2 || id == 5 || id == 6 || id == 40;
    }

    void LogView(uint32_t id, bool widened)
    {
        static std::atomic<uint64_t> seen{ 0 };
        if (id >= 64 || (seen.fetch_or(1ull << id) & (1ull << id)))
            return;
        uint32_t target = ByteSwap(*static_cast<const uint32_t*>(g_memory.Translate(kViewTable + id * kViewStride + kViewTarget)));
        uint32_t w = 0, h = 0;
        if (target)
        {
            w = ByteSwap(*static_cast<const uint32_t*>(g_memory.Translate(target + 16)));
            h = ByteSwap(*static_cast<const uint32_t*>(g_memory.Translate(target + 20)));
        }
        fprintf(stderr, "[uw] view %u: target %ux%u%s\n", id, w, h, widened ? ", widened" : "");
    }

    void ScaleRegister(PPCRegister& f, float scale)
    {
        f.f64 = double(float(f.f64) * scale);
    }

    // About the screen centre (0.5 of it), for the mirror's 0..1 edges.
    void ScaleAboutCentre(PPCRegister& f, float scale)
    {
        f.f64 = double(0.5f + (float(f.f64) - 0.5f) * scale);
    }

    // sub_8226AF50's cache of its last call (view, matrix, object): ProStreet's
    // place for Most Wanted's game::front_end::kMatrixCache.
    constexpr uint32_t kMatrixCache = 0x82BFD5C4;
    constexpr uint32_t kMatrixCachePointer = kMatrixCache + 4;
    game::front_end::MatrixTracker s_frontEndMatrix;  // the game's render thread
    const bool s_frontEndResend = [] {
        const char* v = std::getenv("NFSMW_FE_RESEND");
        return !(v && v[0] == '0');
    }();

    // The hook changed the front end's matrix: clear the cached matrix
    // pointer so the next queued draw uploads it.
    void ResendIfChanged(float x, float y)
    {
        if (!s_frontEndMatrix.Changed(x, y) || !s_frontEndResend)
            return;
        *static_cast<uint32_t*>(g_memory.Translate(kMatrixCachePointer)) = 0;
        static uint32_t logged = 0;
        if (++logged <= 16)
            fprintf(stderr, "[uw] front-end matrix x %.4f y %.4f: sent to the game again%s\n", x, y,
                logged == 16 ? " (no more of these lines)" : "");
    }
}

namespace game
{
    void SetAspectScale(float scaleX, float scaleY)
    {
        const Scales next{ std::clamp(scaleX, 0.1f, 1.0f), std::clamp(scaleY, 0.5f, 1.0f) };
        const Scales last = s_scales.exchange(next, std::memory_order_relaxed);
        if (last.x != next.x || last.y != next.y)
            fprintf(stderr, "[uw] view scale x %.4f y %.4f\n", next.x, next.y);
    }

    float AspectScaleX()
    {
        return s_scales.load(std::memory_order_relaxed).x;
    }

    float AspectScaleY()
    {
        return s_scales.load(std::memory_order_relaxed).y;
    }

    bool HooksActive()
    {
        return Active();
    }

    bool TallHooksActive()
    {
        return Active();
    }

    float FrontEndScaleX()
    {
        return s_frontEndX.load(std::memory_order_relaxed);
    }

    float FrontEndScaleY()
    {
        return s_frontEndY.load(std::memory_order_relaxed);
    }

    // Carbon's pulsing minimap icon has no ProStreet counterpart known:
    // Most Wanted's rule.
    bool FrontEndParksByCentre()
    {
        return false;
    }
}

void UltrawideProjection(PPCRegister& r7, PPCRegister& f5, PPCRegister& f11)
{
    if (!Active())
        fprintf(stderr, "[uw] 3D views drawn: Fill Screen on\n");
    s_lastProjectionNs.store(NowNs(), std::memory_order_relaxed);
    const Scales scale = s_scales.load(std::memory_order_relaxed);
    const uint32_t id = ByteSwap(*static_cast<const uint32_t*>(g_memory.Translate(r7.u32 + 8)));
    const bool widen = WidenedViewId(id);
    LogView(id, widen);
    if (!widen || (scale.x == 1.0f && scale.y == 1.0f))
        return;
    if (scale.x != 1.0f)
        ScaleRegister(f5, scale.x);
    if (scale.y != 1.0f)
        ScaleRegister(f11, scale.y);  // its magnitude: negated into P[1][1] after this
}

void UltrawideFrontEnd(PPCRegister& f13, PPCRegister& f0)
{
    const Scales scale = Active() ? s_scales.load(std::memory_order_relaxed) : Scales{ 1.0f, 1.0f };
    if (scale.x != 1.0f)
        ScaleRegister(f13, scale.x);
    if (scale.y != 1.0f)
        ScaleRegister(f0, scale.y);
    s_frontEndX.store(float(f13.f64), std::memory_order_relaxed);
    s_frontEndY.store(float(f0.f64), std::memory_order_relaxed);
    ResendIfChanged(float(f13.f64), float(f0.f64));
}

// The mirror's and the picture-in-picture's rectangle (sub_82276488): f1
// left edge, f2 top or bottom edge, f3 signed width, f4 height.
PPC_FUNC(sub_82276488)
{
    const Scales scale = s_scales.load(std::memory_order_relaxed);
    if (scale.x != 1.0f)
    {
        ScaleAboutCentre(ctx.f1, scale.x);
        ScaleRegister(ctx.f3, scale.x);
    }
    if (scale.y != 1.0f)
    {
        ScaleAboutCentre(ctx.f2, scale.y);
        ScaleRegister(ctx.f4, scale.y);
    }
    __imp__sub_82276488(ctx, base);
}
