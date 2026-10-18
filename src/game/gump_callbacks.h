// Button click handlers whose receiving class is not identified yet. Both are __thiscall on the gump the
// button reports to; they are grouped here only so they can be members of a CGump-derived class.
// ? Replace CGumpCallbacks by the real class once 0x00472940 (skill gump) and 0x00479500 / 0x0047a4a0
//   (text / tips gumps) are ported.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CGumpCallbacks : public CGump {
public:
    // 0x00412210: sends packet 0x24 with the text "<unk_08> 0" through the object at 0x0069e0e0, and
    // remembers the id in 0x004a8f70. Used by the skill gump row buttons (art 0x837).
    // ? meaning of the packet and of +0x08 (CGump::unk_08) is not settled by pass2.
    void __thiscall SendIdRequest();
    // 0x00412330: creates an 0xb8-byte backdrop child gump with height 0xdc below this gump and gives it a
    // width*height*2 byte buffer tagged "gump background". Used by the text / tips gumps.
    void __thiscall AddBackdropChild();
};

static_assert(sizeof(CGumpCallbacks) == 0x94, "CGumpCallbacks adds no fields");
