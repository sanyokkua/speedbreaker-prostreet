// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// NFS: ProStreet's Direct3D GPU waits: Carbon's hooks (game/carbon/d3d_waits.cpp,
// included below) on ProStreet's D3D. sub_8286B308 is the GPU hang check,
// Carbon's sub_826E0088 instruction for instruction (the only function with
// `cmplwi cr6,r11,5000`; 10 call sites: fence, ring space, idle), with the
// same arguments: args[0] the device, args[8] the GPU counter as last seen,
// args[12] its time. ProStreet's device is 128 bytes bigger in front of these:
// the pointer to the counter the GPU writes is at +10896, the last value
// issued at +10908. sub_82853E70 is the wait for a fence (r3 the device, r4 the
// fence value; Carbon's sub_826DBBB0). Without the back-off the game's main
// thread spun a whole core while it waited for the GPU.
#define SB_D3D_HANG_CHECK sub_8286B308
#define SB_D3D_FENCE_WAIT sub_82853E70
#define SB_D3D_COUNTER_PTR 10896
#define SB_D3D_ISSUED 10908
#include "../carbon/d3d_waits.cpp"
