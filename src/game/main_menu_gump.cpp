// CMainMenuGump constructor, destructor, hit test and per-frame update. (Porting pass 2, batch 08.)
// The SEH frames of the original (try levels around operator new / the constructor) are omitted.
#include "main_menu_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "globals.h"
#include "crt.h"
#include "dumb_gump.h"
#include "gump.h"
#include "char_flow_gumps.h"
#include "small_methods_u1_ports.h"
#include "char_flow_gumps_ports.h"
#include "vtables.h"


// 0x0043d0a0: CDumbGump constructor, RET 0x20, (x, y, a3, w, h, -1, 0, -1). Not ported.
#define Orig_DumbGumpCtor(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CGump *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, a3, a4, a5, a6, (const void *)(a7), a8))

// 0x00423180  size 0x494
// Builds frame0 from art 0x3f2 at (0x6a, 0x75), six buttons, clears the back buffer and draws the first frame,
// with or without the fade-in overlay (art 0x578 as a full-screen gump).
CMainMenuGump::CMainMenuGump()
{
    CButtonGump *btn;
    int clip[4];

    name = const_cast<char *>("MainMenu gump");
    dirty = 0;
    unk_50 = 0x3f2;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x3f2, &width, &height);
    x = 0x6a;
    y = 0x75;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    // Button 1: handler 0x00411010
    btn = new CButtonGump(0x91 - x, 0xc7 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CGump_00411010::Run), 0x3f6, 0x3f7, 0);
    btn->artHover = 0x3f8;
    gfx::AddChild(btn, this);

    // Button 2: handler 0x00411090
    btn = new CButtonGump(0x101 - x, 0x82 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&U1_DocumentationButton), 0x3fc, 0x3fd, 0);
    btn->artHover = 0x3fe;
    gfx::AddChild(btn, this);

    // Button 3: handler 0x004110d0
    btn = new CButtonGump(0x109 - x, 0x9b - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&U1_RegistrationButton), 0x3ff, 0x400, 0);
    btn->artHover = 0x401;
    gfx::AddChild(btn, this);

    // Button 4: handler 0x00410ef0
    btn = new CButtonGump(0xfd - x, 0xbb - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CF_ShowAcctLogin), 0x3f3, 0x3f4, 0);
    btn->artHover = 0x3f5;
    gfx::AddChild(btn, this);

    // Button 5: handler 0x004110e0
    btn = new CButtonGump(0x19f - x, 0xc7 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CF_LaunchWelcomeSplash), 0x3f9, 0x3fa, 0);
    btn->artHover = 0x3fb;
    gfx::AddChild(btn, this);

    // Button 6: handler 0x00410db0
    btn = new CButtonGump(0x119 - x, 0x11c - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&U1_ExitButton), 0x402, 0x403, 0);
    btn->artHover = 0x404;
    gfx::AddChild(btn, this);

    memset(g_backBuffer, 0, 0x25800 * 4);
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = 0x280;
    clip[3] = 0x1e0;
    if (g_introFade != 0) {
        gfx::DrawArtClipped(gfx::artPlain(), 0x587, clip, 0, 0, g_backBuffer, 0x280, 0x1e0);
        CGump *ov;
        ov = new CDumbGump(0, 0, 0, 0x280, 0x1e0, -1, (const void *)(0), -1);
        overlay = ov;
        gfx::DrawArt(gfx::artPlain(), 0x578, 0, 0, gfx::bitmapOf(ov) + 2, ov->width, ov->height);
        gfx::AddChild(overlay, nullptr);
        fadeSteps = 0x14;
        return;
    }
    gfx::DrawArtClipped(gfx::artPlain(), 0x3e8, clip, 0, 0, g_backBuffer, 0x280, 0x1e0);
    fadeSteps = 0;
    overlay = nullptr;
}
HOOK_CTOR(0x00423180, CMainMenuGump, ());

// 0x00423650  size 0x9b
CMainMenuGump::~CMainMenuGump()
{
    if (overlay != nullptr)
        PORT_VSLOT1(overlay, kGumpSlot_DeletingDtor, void, int)(1);
    if (gfx::bitmapOf(this) != nullptr) {
        gfx::FreeTagged(gfx::bitmapOf(this), "MainMenu frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "MainMenu framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00423650, CMainMenuGump);

HOOK_VTABLE(0x0049e278, CMainMenuGump);
HOOK_SDD(0x00423620, CMainMenuGump);

// 0x004236f0  size 0x4d
int __thiscall CMainMenuGump::HitTest(int px, int py)
{
    if (visible != 0 && fadeSteps == 0) {
        uint16_t *bmp = gfx::bitmapOf(this);
        int w = bmp[0];
        int h = bmp[1];
        if (px >= 0 && py >= 0 && px < w && py < h)
            return 1;
    }
    return 0;
}
HOOK_VMETHOD(0x004236f0, CMainMenuGump, HitTest);

// 0x00423740  size 0xef
// While fading: raise the overlay, and in the last 14 steps clear its bitmap and redraw art 0x587 - steps.
// When no fade is left: draw the finished menu to the back buffer and delete the overlay.
void __thiscall CMainMenuGump::Update()
{
    int clip[4];

    if (fadeSteps != 0) {
        overlay->CGump::BringToFront(0);   // CGump_BringToFront
        if (fadeSteps < 0xf) {
            memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(overlay)) + 4, 0, 0x25800 * 4);
            PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, void *, int, int)(gfx::artPlain(), 0x587 - fadeSteps, 0, 0, gfx::bitmapOf(overlay) + 2, overlay->width, overlay->height);
        }
        fadeSteps = fadeSteps - 1;
        return;
    }
    if (overlay != nullptr) {
        clip[0] = 0;
        clip[1] = 0;
        clip[2] = 0x280;
        clip[3] = 0x1e0;
        PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, void *, int, const int *, int, int, void *, int, int)(gfx::artPlain(), 0x3e8, clip, 0, 0, g_backBuffer, 0x280, 0x1e0);
        if (overlay != nullptr)
            PORT_VSLOT1(overlay, kGumpSlot_DeletingDtor, void, int)(1);
        overlay = nullptr;
    }
}
HOOK_VMETHOD(0x00423740, CMainMenuGump, Update);
