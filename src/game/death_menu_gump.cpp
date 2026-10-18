// CDeathMenuGump (0x0043e770 .. 0x0043ebc0). The SEH frames of the constructor and destructor are omitted.
#include "death_menu_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "globals.h"
#include "gump_frame.h"
#include "crt.h"
#include "gump_base_ops_ports.h"
#include "mouse_manager.h"
#include "char_creation_pages.h"
#include "vtables.h"

#define g_mouseManager GLOBAL_PTR(void, g_mouse_storage)
// 0x0043b590 RectClip: ECX = clip, EDX = &rect, stack = &out, RET 4.
#define RectClip(a1, a2, a3) (GumpRectClip(a1, (const int *)(a2), a3))

// 0x0043e770  size 0x247
// Frame0 is the 9-slice frame of art 0x13ec with the two captions; two buttons (0x13ba / 0x13bb) at (0x14, 0x14)
// and (0x3c, 0x14) send packet 0x2c through 0x00411990 / 0x004119d0.
CDeathMenuGump::CDeathMenuGump()
{
    int clip[4];
    int textW;
    int textX;
    int textH;

    name = const_cast<char *>("DeathMenu gump");
    dirty = 0;
    unk_50 = 0x13ec;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = 0x118;
    height = 0x64;
    x = 0xb4;
    y = 0xbe;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    PaintGumpFrame(unk_50, gfx::bitmapOf(this), 0);

    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = height;
    gfx::TextMeasure(1, "Aq", &textW, &textX, &textH);
    gfx::TextDraw(0x3c, textH + 0x14, 1, 0x386, "Play As Ghost", gfx::bitmapOf(this) + 2, width, clip);
    gfx::TextDraw(0x3c, textH + 0x3c, 1, 0x386, "Resurrect With Penalties", gfx::bitmapOf(this) + 2, width, clip);

    CButtonGump *btn;
    btn = new CButtonGump(0x14, 0x14, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CGumpHandlers04::SendPacket2cAndDelete), 0x13ba, 0x13bb, 0);
    gfx::AddChild(btn, this);

    btn = new CButtonGump(0x3c, 0x14, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CGumpHandlers04::SendPacket2cOne), 0x13ba, 0x13bb, 0);
    gfx::AddChild(btn, this);
}
HOOK_CTOR(0x0043e770, CDeathMenuGump, ());

HOOK_VTABLE(0x0049f1b8, CDeathMenuGump);
HOOK_SDD(0x0043e9c0, CDeathMenuGump);

// 0x0043e9f0  size 0x91
CDeathMenuGump::~CDeathMenuGump()
{
    void *bmp = gfx::bitmapOf(this);
    if (bmp != nullptr) {
        gfx::FreeTagged(bmp, "DeathMenu frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "DeathMenu framedata");
        frameData = nullptr;
    }
    if (g_scrollChoiceGump == this)
        g_scrollChoiceGump = nullptr;
}
HOOK_DTOR(0x0043e9f0, CDeathMenuGump);

// 0x0043ea90  size 0xcc
// Same shape as 0x0043de30: frame0 at the absolute origin when the clip overlaps it, then the children.
void __thiscall CDeathMenuGump::Draw(int *clip)
{
    int rect[4];
    int out[4];

    if (visible == 0)
        return;

    int absX = 0;
    int absY = 0;
    for (CGump *g = this; g != nullptr; g = g->parent) {
        absX += g->x;
        absY += g->y;
    }

    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = absX;
    rect[1] = absY;
    rect[2] = absX + bmp[0];
    rect[3] = absY + bmp[1];

    if (clip[2] > absX && clip[0] < rect[2] && clip[3] > absY && clip[1] < rect[3]) {
        RectClip(clip, rect, out);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, out, bmp[0], bmp[1], absX, absY, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, int *)(out);
    }
}
HOOK_VMETHOD(0x0043ea90, CDeathMenuGump, Draw);

// 0x0043eb60  size 0x43
int __thiscall CDeathMenuGump::HitTest(int mx, int my)
{
    if (visible != 0) {
        uint16_t *bmp = gfx::bitmapOf(this);
        int w = bmp[0];
        int h = bmp[1];
        if (mx >= 0 && my >= 0 && mx < w && my < h)
            return 1;
    }
    return 0;
}
HOOK_VMETHOD(0x0043eb60, CDeathMenuGump, HitTest);

// 0x0043ebb0  size 0xe
void __thiscall CDeathMenuGump::MouseDown(int mx, int my)
{
    (void)mx;
    (void)my;
    CGump_BringToFront(this, 0);
}
HOOK_VMETHOD(0x0043ebb0, CDeathMenuGump, MouseDown);

// 0x0043ebc0  size 0x13
// 0x00446840 on the mouse manager (0x0068d2e0): set cursor 0x11.
void __thiscall CDeathMenuGump::MouseOver(int mx, int my)
{
    (void)mx;
    (void)my;
    reinterpret_cast<CMouseManager *>(g_mouseManager)->SetMouseCursor(0x11);
}
HOOK_VMETHOD(0x0043ebc0, CDeathMenuGump, MouseOver);

// Own copies of bodies shared with other classes in the original (the linker folds them).
void __thiscall CDeathMenuGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    g_dragGump = nullptr;
}

void __thiscall CDeathMenuGump::RButtonUp(int mx, int my)
{
    (void)mx;
    (void)my;
    g_dragGump = nullptr;
}

