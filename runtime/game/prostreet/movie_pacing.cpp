// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet's movies are EA VP6 (Movies/*.vp6), decoded by the game,
// not the XDK's XMV player that Most Wanted's and Carbon's movie pacing hook
// (game/movie_pacing.h): no XMV frames to report.
#include <stdafx.h>
#include <game/movie_pacing.h>

namespace game
{
    int64_t LastMovieFrameNs() { return 0; }
}
