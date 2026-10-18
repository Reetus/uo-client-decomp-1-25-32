#include "button_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "vtables.h"

// The SEH frame of the original (try level around the base constructor) is omitted.

// 0x00412fb0  size 0x126
// Button constructor: base CGump ctor, then art-sized frame and a zeroed 2-slot frame data block.
CButtonGump::CButtonGump(int x_, int y_, int p3, int p4, int p5, int p6, int artUp, int artDown_, int p9)
{
    (void)p3;   // not read by the original
    x = x_;
    y = y_;
    param_94 = p4;
    callback = p6;
    name = const_cast<char *>("Button gump");
    unk_c8 = 0;
    flag_b0 = 0;
    unk_ac = 0;
    unk_b8 = 0;
    unk_bc = 0;
    unk_c0 = 1;
    unk_50 = artUp;
    artDown = artDown_;
    unk_a8 = 0;
    artHover = 0;
    param_b4 = p9;
    owner = nullptr;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    if (artUp != 0) {
        gfx::ArtSize(artUp, &width, &height);
    } else if (artDown_ != 0) {
        gfx::ArtSize(artDown_, &width, &height);
    }
    rowStride = width;
    frameIndex = p5;
    frameCount = 2;
    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CButtonGump"));
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) * 8);
}
HOOK_CTOR(0x00412fb0, CButtonGump, (int, int, int, int, int, int, int, int, int));
