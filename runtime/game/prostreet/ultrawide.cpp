// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet: no ultrawide hooks yet (game/ultrawide.h). Most Wanted's
// and Carbon's patch their own projection, HUD matrix and mirror by address;
// ProStreet's are still to be found. HooksActive() stays false, so the
// presenter letterboxes or pillarboxes the 16:9 frame instead of stretching
// it; the scales are kept so the presenter reads back what it set.
#include <stdafx.h>
#include <game/ultrawide.h>

#include <atomic>

namespace
{
    std::atomic<float> s_scaleX{ 1.0f }, s_scaleY{ 1.0f };
}

namespace game
{
    void SetAspectScale(float scaleX, float scaleY)
    {
        s_scaleX.store(scaleX, std::memory_order_relaxed);
        s_scaleY.store(scaleY, std::memory_order_relaxed);
    }
    float AspectScaleX() { return s_scaleX.load(std::memory_order_relaxed); }
    float AspectScaleY() { return s_scaleY.load(std::memory_order_relaxed); }
    bool HooksActive() { return false; }
    bool TallHooksActive() { return false; }
    float FrontEndScaleX() { return 0.0f; }  // no front-end frame seen (the hook's)
    float FrontEndScaleY() { return 0.0f; }
    bool FrontEndParksByCentre() { return false; }
}
