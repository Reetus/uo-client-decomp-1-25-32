// CUnicodeBarkGump: a CBarkGump whose text is a 16-bit string drawn with the unifont routines.
// vtable 0x0049c230, allocated with operator new(0xe0). Constructor 0x00402850.
#pragma once

#include <cstddef>
#include <cstdint>

#include "bark_gump.h"

class CUnicodeBarkGump : public CBarkGump {
public:
    // 0x00402850: stack args (speaker, text, hue, font, extra, param5), RET 0x18. `text` is a 16-bit
    // string; `extra` is stored at +0xdc unchanged. As CBarkGump::Ctor otherwise.
    CUnicodeBarkGump(CObj *speaker, uint16_t *text, uint16_t hue, uint16_t font, int32_t extra, CGump *param5);
    // 0x00402e70: RET 4
    // 0x00402ea0: sets the vtable and tail-calls CBarkGump::Dtor
    ~CUnicodeBarkGump();
    // 0x00402eb0: redraw the wrapped 16-bit text into the bitmap and outline it. RET 8.
    virtual void RenderText(int a, int *b);

    int32_t unk_dc;   // +0xdc  the fifth constructor argument
};

static_assert(offsetof(CUnicodeBarkGump, unk_dc) == 0xdc, "CUnicodeBarkGump::unk_dc");
static_assert(sizeof(CUnicodeBarkGump) == 0xe0, "CUnicodeBarkGump size (operator new(0xe0))");
