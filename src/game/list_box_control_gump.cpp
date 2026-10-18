#include "list_box_control_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "vtables.h"

// The SEH frames of the original (handlers 0x499738 / 0x499758) are omitted.

// 0x00420090  size 0x136
// Constructor: base ctor, art-sized frame (height of art + height of art + 4), frame data with no frame0 bitmap yet.
CListBoxControlGump::CListBoxControlGump(int x_, int y_, unsigned int artId, int unused4, int unused5, int16_t font_,
                                         int16_t hue_, unsigned int rows_)
{
    (void)unused4;
    (void)unused5;
    int dummyW;
    int h2;

    y = y_;
    x = x_;
    name = const_cast<char *>("ListBoxControl gump");
    dirty = 0;
    unk_50 = static_cast<int32_t>(artId & 0xffff);
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(unk_50, &width, &height);
    gfx::ArtSize(unk_50 + 4, &dummyW, &h2);
    height = height + h2;
    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CListBoxControlGump"));
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = 0;
    font = static_cast<uint16_t>(font_);
    hue = static_cast<uint16_t>(hue_);
    unk_a8 = 0;
    unk_9c = 0;
    selected = -1;
    rows = static_cast<int32_t>(rows_ & 0xff);
    if ((rows_ & 0xff) != 0)
        hasScroll = 1;
    else
        hasScroll = 0;
    unk_94 = 0;
    visible = 0;
}
HOOK_CTOR(0x00420090, CListBoxControlGump, (int, int, unsigned int, int, int, int16_t, int16_t, unsigned int));

// 0x00420200  size 0x8b
// Destructor body: frees frame0 and the frame data, then the base destructor.
CListBoxControlGump::~CListBoxControlGump()
{
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "ListBoxControl frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "ListBoxControl framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00420200, CListBoxControlGump);
