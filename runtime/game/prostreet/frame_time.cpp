// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet: no frame-time lock yet (game/frame_time.h). The game runs
// on its own measured frame times; the hook on its main loop (Carbon's:
// game/carbon/frame_time.cpp) is still to be found.
#include <stdafx.h>
#include <game/frame_time.h>

namespace game
{
    FrameTimeStats GetFrameTimeStats() { return {}; }
    double FrameClockMs() { return 0.0; }
}
