#include "spell_icon_gump.h"

#include <cstdio>
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "book_gump.h"
#include "globals.h"
#include "crt.h"
#include "gump.h"
#include "app_session_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "gump_base_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "vtables.h"


// 0x0040c660  size 0x15c
// CSpellIconGump(x, y, spell). The width is rounded up to a multiple of 8
// (the original stores the row stride back into the width as well).
CSpellIconGump::CSpellIconGump(int posX, int posY, int spellIndex)
{
    x = posX;
    y = posY;
    spell = spellIndex;
    int artId = spellIndex + 0x8c0;
    name = const_cast<char *>("spellicon gump");
    dirty = 0;
    dragging = 0;
    unk_60 = 0;
    owner = nullptr;
    unk_50 = artId;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    BOOK_ArtSize(artId, &width, &height);
    rowStride = width + 7;
    rowStride &= ~7;
    width = rowStride;
    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CStatusGump");
    frameData = fd;
    memset(fd, 0, frameCount * 8);
    frameData[1] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(FrameData_Alloc(g_frameDataAllocator, width * height * 2 + 4, "status frame0")));
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    memset(bmp + 4, 0, width * height * 2);
    bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    *reinterpret_cast<uint16_t *>(bmp + 0) = static_cast<uint16_t>(width);
    bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    *reinterpret_cast<uint16_t *>(bmp + 2) = static_cast<uint16_t>(height);
    bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    BOOK_DrawArt(BOOK_ART_PLAIN, unk_50, 0, 0, bmp + 4, *reinterpret_cast<uint16_t *>(bmp + 0),
                 *reinterpret_cast<uint16_t *>(bmp + 2));
}
HOOK_CTOR(0x0040c660, CSpellIconGump, (int, int, int));

HOOK_VTABLE(0x0049c978, CSpellIconGump);
HOOK_SDD(0x0040c7e0, CSpellIconGump);

// 0x0040c810  size 0x8b
// ~CSpellIconGump: frees the frame bitmap and the frame table, then the CGump destructor.
CSpellIconGump::~CSpellIconGump()
{
    void *bmp = reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1]));
    if (bmp != nullptr) {
        BOOK_FrameFree(g_frameDataAllocator, bmp, "spellicon frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        BOOK_FrameFree(g_frameDataAllocator, frameData, "spellicon framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0040c810, CSpellIconGump);

// 0x0040c8a0  size 0x66
// Mouse down: remember the press point, bring to front, start dragging and count the click.
void __thiscall CSpellIconGump::MouseDown(int mouseX, int mouseY)
{
    unk_6c = mouseX;
    unk_70 = mouseY;
    this->CGump::BringToFront(0);   // bring to front
    dragging = 1;
    unk_b0 = 1;
    this->CGump::BringToFront(0);   // bring to front (again, as the original does)
    if (clickState == 1) {
        clickState = 2;
        return;
    }
    clickState = 1;
    clickTick = GetTickCount();
}
HOOK_VMETHOD(0x0040c8a0, CSpellIconGump, MouseDown);

// 0x0040c910  size 0x74
// Mouse up: a drag that Tick turned into dragStarted just ends; the second click of a double click
// sends "<spell + 1>" to the server (string packet 0x56 through 0x0044df70).
void __thiscall CSpellIconGump::MouseUp(int mouseX, int mouseY)
{
    (void)mouseX;
    (void)mouseY;
    char text[0x14];
    dragging = 0;
    if (dragStarted != 0) {
        dragStarted = 0;
        clickState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (clickState == 2) {
        int s = spell;
        clickState = 0;
        sprintf(text, "%d", s + 1);
        SendTextPacket(0x56, text);
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0040c910, CSpellIconGump, MouseUp);

// 0x0040c9b0  size 0x2c
void __thiscall CSpellIconGump::MouseMove(int mouseX, int mouseY)
{
    if (dragging != 0)
        Move(mouseX - unk_6c, mouseY - unk_70, 1, nullptr, 1);
}
HOOK_VMETHOD(0x0040c9b0, CSpellIconGump, MouseMove);

// 0x0040c9e0  size 0x70
// Tick: while dragging, count ticks (reset at 2); a first click that no second click followed within
// the double-click time is dropped, and starts a drag if the button is still down.
void __thiscall CSpellIconGump::Tick()
{
    if (dragging != 0) {
        unk_b0 = unk_b0 + 1;
        if (unk_b0 == 2)
            unk_b0 = 0;
    }
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState == 1 && now - clickTick > dbl) {
        clickState = 0;
        if (dragging != 0)
            dragStarted = 1;
    }
}
HOOK_VMETHOD(0x0040c9e0, CSpellIconGump, Tick);

// 0x0040ca50  size 0x9a
// Draw: blit the frame bitmap at the screen origin if the gump manager accepts the clip. No children.
void __thiscall CSpellIconGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    this->CGump::LocalToScreen(&sx, &sy);   // CGump::LocalToScreen
    uint16_t *bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    int rect[4];
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp[0];
    rect[3] = sy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) != 0) {
        PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, const int *, int, int, int, int, void *, int, int)(
            BOOK_ART_PLAIN, bmp + 2, clip, bmp[0], bmp[1], sx, sy, g_backBuffer, 0x280, 0x1e0);   // blit frame
    }
}
HOOK_VMETHOD(0x0040ca50, CSpellIconGump, Draw);

// 0x0040caf0  size 0x1
// The empty procedure: default of slots OnTab/OnOk/OnCancel/Update/Slot60 (CGump), and the entry destructor body of
// part_list_ops.cpp (called on a PartEntry, not a gump: so a plain function, not a member).
void __fastcall Nop_0040caf0(void *)
{
}
HOOK_FUNC(0x0040caf0, Nop_0040caf0);
