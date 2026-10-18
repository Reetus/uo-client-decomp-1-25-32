// CTextGump constructor (0x00479500) and CreateBarkGump (0x00479480). (Porting pass 2, batch 29.)
// The SEH frames of the original (try levels around operator new) are omitted.
#include "text_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "bark_gump.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "font_manager.h"
#include "crt.h"
#include "scroll_thumb_gump.h"
#include "book_gump.h"
#include "gump_callbacks.h"
#include "ui_callbacks_ports.h"
#include "vtables.h"
#include "globals.h"


// 0x00479480  size 0x72  RET 0xc (hue, font, text). When the font exists: set g_barkState and build a bark gump
// (no speaker, no parent). Returns the new gump, or 0 when the font is missing.
CBarkGump *__stdcall CreateBarkGump(int hue, int font, char *text)
{
    if (g_fontMgr.HasFont(font) == 0)   // 0x004320a0
        return nullptr;
    g_barkState = 1;
    return new CBarkGump(nullptr, text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), nullptr);   // 0x004012a0
}
HOOK_FUNC(0x00479480, CreateBarkGump);

// 0x00479500  size 0x45f  RET 8 (x, y). The text window: frame 0 (the visible bitmap) and frame 1 (a copy of art 0x830
// drawn once), the scroll thumb at +0xc0, the buttons 0x89 (art 0x82d), 0x89 (0x82e, hover 0x82f), +0xc4 (0x82c),
// and the two arrows (0x824 / 0x825). The art sizes come from gump art 0x820 / 0x830 / 0x82e.
CTextGump::CTextGump(int x_, int y_)
{
    int artW;
    int artH;
    CButtonGump *btn;
    CScrollThumbGump *thumb;

    cur = g_textListHead;
    lineOffset = 0;
    repeat = 0;
    x = x_;
    y = y_;
    name = g_004af95c;   // "text gump"
    dirty = 0;
    unk_58 = 0;
    unk_a0 = 0;
    held = 0;
    unk_98 = 0;
    unk_94 = 0;
    pressState = 0;
    pressTick = 0;
    unk_60 = 0;
    unk_50 = 0x820;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;

    gfx::ArtSize(0x820, &width, &height);   // 0x00433e70
    int stride = (width + 7) & ~7;
    height = 0x12c;
    rowStride = stride;
    width = stride;
    gfx::ArtSize(0x830, &unk_24, &unk_28);   // 0x00433e70
    frameIndex = 0;
    frameCount = 2;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CTextGump"));   // 0x004462f0
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);

    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "text frame0"));   // frame 0
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    uint16_t *frame0 = gfx::bitmapOf(this);
    frame0[0] = static_cast<uint16_t>(width);
    frame0[1] = static_cast<uint16_t>(height);

    frameData[3] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(unk_24 * unk_28 * 2 + 4, "text frame1")));   // frame 1
    uint16_t *frame1 = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    memset(reinterpret_cast<uint8_t *>(frame1) + 4, 0, static_cast<size_t>(unk_24 * unk_28 * 2));
    frame1[0] = static_cast<uint16_t>(unk_24);
    frame1[1] = static_cast<uint16_t>(unk_28);
    gfx::DrawArt(gfx::artPlain(), 0x830, 0, 0, reinterpret_cast<uint8_t *>(frame1) + 4, frame1[0], frame1[1]);   // 0x00433ed0

    textLeft = 0x26;
    textTop = 0x44;
    unk_b8 = width - 0x31;
    if (unk_b8 % 2 == 0)
        unk_b8 = unk_b8 - 1;
    unk_bc = height - 0x4e;
    this->CTextGump::ScrollToEnd();                       // ScrollToEnd
    this->CTextGump::RedrawText(1, 1);         // RedrawText

    gfx::ArtSize(0x82e, &artW, &artH);   // 0x00433e70

    // scroll thumb
    thumb = new CScrollThumbGump(0x100, 0x48, 1, height - artH - 0x38, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0x1f);
    childC0 = thumb;
    thumb->LinkInto(this, 1);   // 0x0043bcc0
    thumb->PlaceFromTextList();   // 0x00413b20 (on the control at +0xc0)

    // button 0x82d at (0x89, 0)
    btn = new CButtonGump(0x89, 0, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CBookGump::Iconify), 0x82d, 0x82d, 0);
    btn->LinkInto(this, 1);   // 0x0043bcc0

    // button 0x82e at (0x89, height - artH - 3) with hover art 0x82f
    btn = new CButtonGump(0x89, height - artH - 3, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CGumpCallbacks::AddBackdropChild), 0x82e, 0x82e, 0);
    btn->artHover = 0x82f;   // ? the original stores through the pointer even when the allocation failed
    btn->LinkInto(this, 1);   // 0x0043bcc0

    // button 0x82c at (0xe9, height - artH - 0x34), kept at +0xc4
    btn = new CButtonGump(0xe9, height - artH - 0x34, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 1, 0, PORT_ADDR(&UiCb_TextScrollToEnd), 0, 0x82c, 0);
    childC4 = btn;
    btn->LinkInto(this, 1);   // 0x0043bcc0

    // up arrow 0x824 at (0xf9, 0x38)
    btn = new CButtonGump(0xf9, 0x38, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&UiCb_TextScrollUp), 0x824, 0x824, 0);
    btn->LinkInto(this, 1);   // 0x0043bcc0

    // down arrow 0x825 at (0xf9, height - artH - 0x34)
    btn = new CButtonGump(0xf9, height - artH - 0x34, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&UiCb_TextScrollDown), 0x825, 0x825, 0);
    btn->LinkInto(this, 1);   // 0x0043bcc0

}
HOOK_CTOR(0x00479500, CTextGump, (int, int));
