// The big client state object at 0x0068dd98 (class name not known yet; the world view gump pointer
// lives at +0x10008). Layout partial; see decomp/pass2/0x00436c70.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "globals.h"

class CClientState {
public:
    // 0x00436c70: create the world map gump (hidden) and the gump of type 0x9c, add the latter
    // to the top-level list.
    void __thiscall CreateWorldGumps();

    uint8_t pad_00000[0x10008];
    CGump *worldMapGump;        // +0x10008
};

static_assert(offsetof(CClientState, worldMapGump) == 0x10008, "CClientState::worldMapGump");

#define g_clientState GLOBAL_AS(CClientState, g_clientState_storage)
