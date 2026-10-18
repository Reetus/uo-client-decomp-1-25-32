// CYesNoGump constructor. (Porting pass 2, batch 07.)
#include "yes_no_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "crt.h"
#include "vtables.h"
#include "globals.h"


// 0x00422e50  size 0x259
// Constructor: centred on the 640x480 screen, frame0 from art 0x816 with the question wrapped at the top,
// a Yes button (callback 1) and a No button (callback 2).
CYesNoGump::CYesNoGump(const char *text, void *yesCallback, void *noCallback, int extra_)
{
    int clip[4];

    g_scrollChoiceGump = this;
    extra = extra_;
    unk_94 = 0;
    name = const_cast<char *>("YesNo gump");
    dirty = 0;
    unk_50 = 0x816;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x816, &width, &height);
    x = 0x140 - width / 2;
    y = 0xf0 - height / 2;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CYesNoGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    clip[0] = 0x21;
    clip[1] = 0x1e;
    clip[2] = 0x94;
    clip[3] = 0x3f;
    gfx::TextDrawWrapped(0x21, 0x1e, 1, 0x386, text, 0x73, gfx::bitmapOf(this) + 2, width, clip, 0, 0);

    // Yes button
    CButtonGump *btn;
    btn = new CButtonGump(0x64, 0x4b, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, static_cast<int>(reinterpret_cast<uintptr_t>(yesCallback)), 0x81a, 0x81b, 0);
    btn->artHover = 0x81c;
    gfx::AddChild(btn, this);

    // No button
    btn = new CButtonGump(0x25, 0x4b, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, static_cast<int>(reinterpret_cast<uintptr_t>(noCallback)), 0x817, 0x818, 0);
    btn->artHover = 0x819;
    gfx::AddChild(btn, this);
}
HOOK_CTOR(0x00422e50, CYesNoGump, (const char *, void *, void *, int));
