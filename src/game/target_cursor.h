// Target cursor completion: the callback stored in DAT_006f4784 by the 0x6C and
// 0x99 handlers is called with the clicked world object in ECX and sends the
// 0x6C response. See decomp/pass2/0x0045d630.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "obj.h"

// A world object seen as the target of a target cursor. It adds no fields; it
// exists so the callback can be a __thiscall member (the callee reads x/y/z at
// +0x20/+0x22/+0x24, the graphic at +0x38 and the serial at +0x78, and asks
// two virtual questions of the object).
class CTargetObj : public CObj {
public:
    // 0x0045d630  size 0x2c9  send the 0x6C target response for this object:
    // Three variants chosen by vtable slots 0x1c and 0x2c (what they ask is not
    // settled); the second one carries the serial from +0x78.
    void __thiscall SendTargetResponse();
};
static_assert(sizeof(CTargetObj) == sizeof(CObj), "CTargetObj adds no fields");

// 0x0045d3d0  size 0x236  send the "cancel" variant of the 0x6C response and clear the targeting state.
void __cdecl TargetCursor_SendCancel();
