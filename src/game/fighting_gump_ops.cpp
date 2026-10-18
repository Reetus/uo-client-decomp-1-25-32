// CFightingGump: destructor, drag handlers and toggle handler (0x00431b40 .. 0x00431cf0).
// The SEH frame of the destructor (try level around the base destructor) is omitted.
#include <windows.h>

#include "../runtime/ported.h"
#include "fighting_gump.h"
#include "crt.h"
#include "vcall.h"
#include "map_world_ops_ports.h"
#include "book_gump.h"
#include "vtables.h"
#include "globals.h"
#include "raw_field.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "gump_base_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "global_views.h"

// 0x00446310 on the allocator object at 0x0064cdac: release block (key, name), RET 8.
#define ReleaseBlock(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), uint32_t, const char *)(a1, a2)
// 0x0040d410: restore notification of a minimised gump (ECX = gump).
#define Gump_Uniconify(self) (reinterpret_cast<CBookGump *>(self)->Uniconify())

HOOK_VTABLE(0x0049e998, CFightingGump);
HOOK_SDD(0x00431b40, CFightingGump);

// 0x00431b70  size 0xa9
// Release the two frame bitmaps (frameData +4 and +0xc) and then the frame data itself, each
// under its debug name; the base destructor runs after.
CFightingGump::~CFightingGump()
{
    GLOBAL_AS(CFightingGump *, g_fightingGump) = nullptr;

    int32_t bmp0 = frameData[1];
    if (bmp0 != 0) {
        ReleaseBlock(g_frameDataAllocator, static_cast<uint32_t>(bmp0), "fightgump frame0 pBMP");   // 0x00446310
        frameData[1] = 0;
    }
    int32_t bmp1 = frameData[3];
    if (bmp1 != 0) {
        ReleaseBlock(g_frameDataAllocator, static_cast<uint32_t>(bmp1), "fightgump frame1 pBMP");   // 0x00446310
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        ReleaseBlock(g_frameDataAllocator, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(frameData)), "fightgump framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00431b70, CFightingGump);

// 0x00431c20  size 0x54
// Drag start. unk_a8: 0 idle, 1 pressed, 2 dragging-from-pressed; unk_a4 is the press time.
void __thiscall CFightingGump::MouseDown(int grabX, int grabY)
{
    CGump_BringToFront(this, 0);   // 0x0043ba70
    unk_9c = 1;
    unk_6c = grabX;
    unk_70 = grabY;
    if (unk_a8 == 1) {
        unk_a8 = 2;
        return;
    }
    unk_a8 = 1;
    unk_a4 = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x00431c20, CFightingGump, MouseDown);

// 0x00431c80  size 0x53
// Drag end (both arguments unused). With unk_94 set it only clears the state; otherwise a gump
// that was in state 2 (unk_a8) is notified through 0x0040d410 when unk_60 is set.
void __thiscall CFightingGump::MouseUp(int unusedA, int unusedB)
{
    (void)unusedA;
    (void)unusedB;
    int32_t prevUnk94 = unk_94;
    unk_9c = 0;
    if (prevUnk94 != 0) {
        unk_94 = 0;
        unk_a8 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_a8 == 2) {
        int32_t notify = unk_60;
        unk_a8 = 0;
        if (notify != 0)
            Gump_Uniconify(this);   // 0x0040d410
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00431c80, CFightingGump, MouseUp);

// 0x00431ce0  size 0x7
// Empty, three stack arguments: the default of slots OnScroll/OnScrollEnd (CGump). Also called by packet 0x43 on a
// non-gump object (inbound_data_updates.cpp), so a plain function.
void __fastcall Nop_00431ce0(void *, void *, int, int, int)
{
}
HOOK_FUNC(0x00431ce0, Nop_00431ce0);

// 0x00431cf0  size 0x82
// A child toggle (virtual slot 0xa8 is true for toggles) was clicked. If it was not selected it
// becomes selected (frame 1) and nothing else happens; otherwise every other toggle child is set
// to frame 0 and the two sliders are shown iff the second toggle is selected.
void __thiscall CFightingGump::OnChildChanged(int child)
{
    CGump *button = reinterpret_cast<CGump *>(child);
    if (decomp::vcall(PORT_RET(int), button, 0xa8) != 0 && button->frameIndex == 0) {
        button->frameIndex = 1;
        return;
    }
    for (CGump *child = lastChild; child != nullptr; child = child->prev) {
        if (decomp::vcall(PORT_RET(int), child, 0xa8) != 0 && child != button)
            child->frameIndex = 0;
    }
    int shown = (static_cast<CGump *>(toggleB)->frameIndex == 1) ? 1 : 0;
    static_cast<CGump *>(sliderA)->visible = shown;
    static_cast<CGump *>(sliderB)->visible = shown;
}
HOOK_VMETHOD(0x00431cf0, CFightingGump, OnChildChanged);

// Own copies of bodies shared with other classes in the original (the linker folds them).
// CFightingGump::Draw (the copy of CTextGump::Draw) lives in gump_slot_copies_text.cpp: its position in a translation unit decides whether the linker folds it.

int __thiscall CFightingGump::HitTest(int localX, int localY)
{
    uint16_t *bmp;
    if (unk_60 != 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[3]));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    if (localX < 0 || localY < 0)
        return 0;
    if (localX >= bmp[0])
        return 0;
    if (localY >= bmp[1])
        return 0;
    return bmp[localY * bmp[0] + localX] != 0;
}

void __thiscall CFightingGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CFightingGump::MouseMove(int mx, int my)
{
    SetCursorForRegion(0);   // 0x00482be0
    int wasHeld = RAWF(int32_t, this, 0x9c);
    if (wasHeld != 0 && RAWF(int32_t, this, 0xa8) != 0) {
        int dx = mx - unk_6c;
        if (dx < 0)
            dx = -dx;
        int dy = my - unk_70;
        if (dy < 0)
            dy = -dy;
        if (dx > 4 || dy > 4) {
            RAWF(int32_t, this, 0x94) = 1;
            RAWF(int32_t, this, 0xa8) = 0;
        }
    }
    if (wasHeld != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}

void __thiscall CFightingGump::Tick()
{
    DWORD now = GetTickCount();
    UINT dct = GetDoubleClickTime();
    if (RAWF(int32_t, this, 0xa8) == 1 && (now - static_cast<DWORD>(RAWF(int32_t, this, 0xa4))) > dct) {
        RAWF(int32_t, this, 0xa8) = 0;
        if (RAWF(int32_t, this, 0x9c) != 0)
            RAWF(int32_t, this, 0x94) = 1;
    }
}

