// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet: no split-screen support yet (game/split_screen.h). The
// predicate reads a game's front-end database by address (Carbon's:
// game/carbon/split_screen.cpp); ProStreet's is still to be found.
#include <stdafx.h>
#include <game/split_screen.h>

namespace game::split_screen
{
    bool Enabled() { return false; }
    bool GameSaysSplit() { return false; }
}
