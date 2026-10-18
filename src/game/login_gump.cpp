#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "input_control_gump.h"
#include "login_gumps.h"
#include "crt.h"
#include "char_ui_gumps.h"
#include "vtables.h"
#include "globals.h"

// Each child is a `new` expression (the original's try levels 1..10 around each allocation). Buttons are dereferenced
// without a NULL check, as in the original.


// 0x00420b90  size 0x70b
// Login gump constructor. Art 0x44c background in a dumb-gump style frame0, five input fields, five buttons.
CLoginGump::CLoginGump()
{
    name = const_cast<char *>("Login gump");
    dirty = 0;
    unk_50 = 0x44c;
    unk_60 = 0;
    unk_94 = 0;
    focusIndex = g_selectedCharSlot;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x44c, &width, &height);
    x = 0x7c;
    y = 0x68;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    // ---- input field 0 (account name) ----
    children[0] = new CInputControlGump(0xcd - x, 0xb5 - y, this, 0x45c, 0x45c, 0, 0x1e, 1, 0x386, 1, 0, 0);
    gfx::AddChild(children[0], this);
    children[0]->artDe = 0x45d;
    children[0]->artFocused = 0x45e;
    children[0]->marginX = 0x16;
    children[0]->marginY = 0;
    children[0]->dropDown = 1;
    children[0]->Redraw();

    // ---- input field 1 (password) ----
    state[0] = -1;
    children[1] = new CInputControlGump(0xd9 - x, 200 - y, this, 0x45f, 0x45f, 0, 0x1e, 1, 0x386, 1, 0, 0);
    gfx::AddChild(children[1], this);
    children[1]->artDe = 0x460;
    children[1]->artFocused = 0x461;
    children[1]->marginX = 0xa;
    children[1]->marginY = 0;
    children[1]->dropDown = 1;
    children[1]->Redraw();

    // ---- input field 2 ----
    state[1] = -1;
    children[2] = new CInputControlGump(0xe0 - x, 0xda - y, this, 0x462, 0x462, 0, 0x1e, 1, 0x386, 1, 0, 0);
    gfx::AddChild(children[2], this);
    children[2]->artDe = 0x463;
    children[2]->artFocused = 0x464;
    children[2]->marginX = 3;
    children[2]->marginY = 0;
    children[2]->dropDown = 1;
    children[2]->Redraw();

    // ---- input field 3 ----
    state[2] = -1;
    children[3] = new CInputControlGump(0xd8 - x, 0xee - y, this, 0x465, 0x465, 0, 0x1e, 1, 0x386, 1, 0, 0);
    gfx::AddChild(children[3], this);
    children[3]->artDe = 0x466;
    children[3]->artFocused = 0x467;
    children[3]->marginX = 0xb;
    children[3]->marginY = 0;
    children[3]->dropDown = 1;
    children[3]->Redraw();

    // ---- input field 4 ----
    state[3] = -1;
    children[4] = new CInputControlGump(0xca - x, 0x101 - y, this, 0x468, 0x468, 0, 0x1e, 1, 0x386, 1, 0, 0);
    gfx::AddChild(children[4], this);
    children[4]->artDe = 0x469;
    children[4]->artFocused = 0x46a;
    children[4]->marginX = 0x19;
    children[4]->marginY = 0;
    children[4]->dropDown = 1;
    children[4]->Redraw();

    // mark the initially focused field
    state[4] = -1;
    children[focusIndex]->focused = 1;
    children[focusIndex]->Redraw();

    CButtonGump *btn;

    // ---- button 0 ----
    btn = new CButtonGump(0xbb - x, 0x86 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharSelectGump::NewCharacter), 0x453, 0x454, 0);
    btn->artHover = 0x455;
    gfx::AddChild(btn, this);

    // ---- button 1 ----
    btn = new CButtonGump(0xb5 - x, 0x11f - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharSelectGump::OpenDeleteDialog), 0x44d, 0x44e, 0);
    btn->artHover = 0x44f;
    gfx::AddChild(btn, this);

    // ---- button 2 ----
    btn = new CButtonGump(0x172 - x, 0x11f - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharSelectGump::OnChangePassword), 0x450, 0x451, 0);
    btn->flag_b0 = 1;
    btn->artHover = 0x452;
    gfx::AddChild(btn, this);

    // ---- button 3 ----
    btn = new CButtonGump(0x171 - x, 0x86 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharSelectGump::Play), 0x459, 0x45a, 0);
    btn->artHover = 0x45b;
    gfx::AddChild(btn, this);

    // ---- button 4 ----
    btn = new CButtonGump(0x11c - x, 0x75 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharSelectGump::Disconnect), 0x456, 0x457, 0);
    btn->artHover = 0x458;
    gfx::AddChild(btn, this);
}
HOOK_CTOR(0x00420b90, CLoginGump, ());
