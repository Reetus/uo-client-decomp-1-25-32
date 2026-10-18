// CPlayCharGump: the password prompt of the character select flow (constructor and destructor).
// (Porting pass 2, batch 07.)
#include "char_flow_gumps.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "globals.h"
#include "input_control_gump.h"
#include "crt.h"
#include "raw_field.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "class_0049c690.h"


// 0x00422a10  size 0x346
// Constructor: frame0 from art 0x474 with the "Password" label and the "Play as %s?" text, a password input
// control (+0x98) that takes the keyboard focus, and two buttons (OK 0x0040eab0 and 0x0040cf60).
CPlayCharGump::CPlayCharGump(const char *charName, int unused)
{
    (void)unused;
    char buf[0x50];
    int clip[4];
    void *mem;

    RAWF(int32_t, this, 0x94) = 0;
    name = const_cast<char *>("PlayChar gump");
    dirty = 0;
    unk_50 = 0x474;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x474, &width, &height);
    x = 0x79;
    y = 0x66;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CPlayCharGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(height * width * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = height;
    gfx::TextDraw(0x3c, 0x7d, 2, 0x386, "Password", gfx::bitmapOf(this) + 2, width, clip);
    sprintf(buf, "Play as %s?", charName);
    gfx::TextDrawWrapped(0x3c, 0x32, 2, 0x386, buf, width - 0x78, gfx::bitmapOf(this) + 2, width, clip, 0, 0);

    // password input control: p = operator new(0x158); obj = p ? ctor(p, ...) : 0
    password = new CInputControlGump(0x3c, 0x82, this, 0x475, 0x475, 0, 0x1e, 1, 0x386, 1, 0, 0);
    password->artDe = 0x476;
    password->artFocused = 0x477;
    password->marginX = 0xc;
    password->marginY = 5;
    gfx::AddChild(password, this);
    password->password = 1;   // mask with '*'
    password->focused = 1;
    GLOBAL_AS(CInputControlGump *, g_focusedInput) = password;
    password->Redraw();

    // button 0 (OK)
    CButtonGump *btn;
    btn = new CButtonGump(0xee - x, 0x124 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CPlayCharGump::Ok), 0x47b, 0x47c, 0);
    btn->artHover = 0x47d;
    gfx::AddChild(btn, this);

    // button 1
    btn = new CButtonGump(0x145 - x, 0x124 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&DeleteGumpCallback), 0x478, 0x479, 0);
    btn->artHover = 0x47a;
    gfx::AddChild(btn, this);
}
HOOK_CTOR(0x00422a10, CPlayCharGump, (const char *, int));

HOOK_VTABLE(0x0049e048, CPlayCharGump);
HOOK_SDD(0x00422d60, CPlayCharGump);

// 0x00422d90  size 0xa9
// Destructor body: makes the gump in 0x006488e8 visible again, deletes the input control, frees frame0 and the frame data.
CPlayCharGump::~CPlayCharGump()
{
    if (g_prevGump != nullptr)
        g_prevGump->visible = 1;
    if (password != nullptr) {
        PORT_VSLOT1(password, 0x00, void, int)(1);
        password = nullptr;
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "PlayChar frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "PlayChar framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00422d90, CPlayCharGump);
