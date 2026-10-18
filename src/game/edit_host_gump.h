// A gump that owns one edit control at +0x11c, created lazily.
// Layout from decomp/pass2/0x004694d0.cpp. ? The real class name is not known
// (it sits next to CStackGump in the original image); the name here is a placeholder.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CEditHostGump : public CGump {
public:
    // 0x004694d0  create the edit control once, seed it with the pending text, attach it
    void __thiscall CreateEditControl();

    uint8_t pad_94[0x9c - 0x94];
    char text[0x11c - 0x9c];    // +0x9c  pending text (size up to the control slot)
    uint8_t *editControl;       // +0x11c  CEdit (0x158 bytes), text buffer at its +0xec
};

static_assert(offsetof(CEditHostGump, text) == 0x9c, "CEditHostGump::text");
static_assert(offsetof(CEditHostGump, editControl) == 0x11c, "CEditHostGump::editControl");
