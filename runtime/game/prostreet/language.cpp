// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet (as Carbon, game/carbon/language.cpp): Settings > Game > Language is Most Wanted's (game/language.h:
// its language table, boot call and movie and speech names; the hooks in
// game/language.cpp). ProStreet plays its disc's own language, and the row is
// hidden (user/settings.cpp kLanguageHidden), so the menu never asks.
#include <stdafx.h>
#include <game/language.h>

namespace game::language
{
    std::string MenuNote(int32_t) { return std::string(); }
    // Most Wanted's PAL (Spanish) disc's language at its first run
    // (main.cpp); no PAL release here: nothing to set.
    void SetLaunchLanguage(int32_t) {}
}
