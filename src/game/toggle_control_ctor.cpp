#include "toggle_control.h"

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

// The SEH frames of the originals (try level around the base constructor / destructor) are omitted.

// 0x00416120  size 0x20f
// Check box with a text label: frame0 is (art width + 4 + text width, art height) with the label drawn after
// the art; Redraw then draws the art for the state.
CToggleControl::CToggleControl(int x_, int y_, int a3, int art, int artChecked_, int font, int hue, const char *text,
                                     int checked_, int artDisabled_, int artDisabledChecked_)
{
    (void)a3;   // not read by the original
    x = x_;
    artChecked = artChecked_;
    artDisabled = artDisabled_;
    y = y_;
    artDisabledChecked = artDisabledChecked_;
    name = const_cast<char *>("ToggleControl gump");
    dirty = 0;
    unk_50 = art;
    artHoverChecked = 0;
    artHover = 0;
    unk_60 = 0;
    disabled = 0;
    hovering = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(art, &width, &height);

    int fontId = font & 0xffff;
    int textW;
    int textX;
    int textH;
    gfx::TextMeasure(fontId, text, &textW, &textX, &textH);   // 0x004327b0
    width = width + (textW + 4);
    // an odd positive width is rounded up
    if ((((abs(width) & 1) ^ (width >> 31)) - (width >> 31)) == 1)
        width = width + 1;

    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CToggleControlGump"));
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) * 8);
    void *bmp = gfx::AllocTagged(height * width * 2 + 4, "togglectrl frame0");
    gfx::setBitmap(this, bmp);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    int artW;
    int artH;
    gfx::ArtSize(unk_50, &artW, &artH);
    int half = (artH - textX) / 2;
    int clip[4];
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = height;
    gfx::TextDraw(artW + 4, half + textH + 2, fontId, hue & 0xffff, text, gfx::bitmapOf(this) + 2, width, clip);   // 0x00432130
    checked = checked_;
    Redraw();   // 0x004163f0
}
HOOK_CTOR(0x00416120, CToggleControl, (int, int, int, int, int, int, int, const char *, int, int, int));

HOOK_VTABLE(0x0049cdd8, CToggleControl);
HOOK_SDD(0x00416330, CToggleControl);

// 0x00416360  size 0x8b
CToggleControl::~CToggleControl()
{
    if (frameData[1] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1]))), "ToggleControl frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "ToggleControl framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00416360, CToggleControl);

