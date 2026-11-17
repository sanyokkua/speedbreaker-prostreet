// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet: the NFSMW_LOG_COVERS=1 caller log (game/cover_callers.h)
// hooks a game's ePoly vertex writer by address (Carbon's sub_824F0418, by
// fingerprint from Most Wanted's); ProStreet's is still to be found. Off.
#include <stdafx.h>
#include <game/cover_callers.h>

namespace game::covers
{
    bool Enabled() { return false; }
    uint64_t QuadKey(const uint32_t*) { return 0; }
    uint64_t Noted() { return 0; }
    bool CallerOf(uint64_t, Chain&) { return false; }
    std::string Describe(const Chain&) { return std::string(); }
}
