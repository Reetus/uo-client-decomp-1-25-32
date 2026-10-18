// CInputGump: the one-line text strip along the bottom of the screen that shows what the player is
// typing (and, when enabled, the frame rate). Ctor 0x00402360, vtable 0x0049c118, allocated by the
// caller with operator new(0x94), so it adds no fields to CGump.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CInputGump : public CGump {
public:
    virtual int IsTag74() { return 1; }
    virtual int IsTag80() { return 0; }
    // 0x00402360: real constructor, stack args (text, hue, font, unicode), RET 0x10. `text` is a char * or, when
    // `unicode` is non-zero, a 16-bit string; hue / font are the dword values the callers push.
    CInputGump(void *text, uint32_t hue, uint32_t font, int unicode);
    // 0x00402430: RET 4
    // 0x00402460: destructor body (frees the text bitmap)
    ~CInputGump();
    void Dtor() { this->CInputGump::~CInputGump(); }
    // 0x004024d0: rebuild the bitmap from an 8-bit string. RET 0xc.
    void __thiscall Redraw(char *text, uint16_t hue, uint16_t font);
    // 0x004026b0: rebuild the bitmap from a 16-bit string, outlined. RET 0xc.
    // `hueIndex` selects the colour from the font hue table (0x58 bytes per entry).
    void __thiscall RedrawUnicode(uint16_t *text, uint16_t hueIndex, uint16_t unused);
};

static_assert(sizeof(CInputGump) == 0x94, "CInputGump size (operator new(0x94) at the call sites)");
