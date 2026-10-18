// Active mobile animations (table of 20 mobiles at 0x004ba2b0) and the missile table
// (20 entries at 0x004b7250), stepped once per tick by 0x00430be0. Methods are on a class
// derived from CMobile so the originals can be hooked as __thiscall members; objects are
// the ordinary CMobile instances.
#pragma once

#include <cstddef>
#include <cstdint>

#include "obj.h"

class CMobileAnimQueue : public CMobile {
public:
    // 0x00430920  size 0x45  put this mobile into the first free slot of the active table
    // (unless it already has one: +0x1d0 != -1); always clears the flag at 0x006f478c.
    void __thiscall RegisterActiveAnim();
    // 0x00430970  size 0xf1  end this mobile's animation: leave the table, drop the attached
    // effect. Returns 1 when the animation was a final (death) one, else sets 0x006f478c and
    // returns 0.
    int __thiscall FinishAnim();
    // 0x00430a70  size 0x16c  advance the animation one step (frame, then sequence, ping-pong
    // when the repeat word is set), finishing it when it runs out.
    void __thiscall AnimStep();
};

// 0x00430900  size 0x11  clear the active-animation table.
void __cdecl MobileAnim_ClearActive();
// 0x00430be0  size 0x5d  per-tick: step every active mobile animation whose tick counter passed
// its limit, and every live missile. (Calling convention not annotated; no arguments.)
void __cdecl MobileAnim_TickAll();
