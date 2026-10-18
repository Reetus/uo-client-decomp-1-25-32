// CGump_0043de30: draw and mouse handlers (0x0043de30, 0x0043df00, 0x0043df40, 0x0043df80).
#include "waiting_gump.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "gump_base_ops_ports.h"
#include "char_flow_gumps.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_ports.h"

// 0x0043b590 RectClip: ECX = clip, EDX = &rect, stack = &out, RET 4.
#define RectClip(a1, a2, a3) (GumpRectClip(a1, (const int *)(a2), a3))
// 0x004342e0: ECX = art object 0x004764c0, EDX = source pixels, stack (clip, w, h, ox, oy, dest, destW, destH), RET 0x20.
#define BlitFrame PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, void *, int, int, int, int, void *, int, int)

// 0x0043de30  size 0xd0
// Draws frame0 at the gump's absolute origin when the clip rectangle overlaps it, then the children.
void __thiscall CGump_0043de30::Draw(int *clip)
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

    uint16_t *bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    rect[0] = absX;
    rect[1] = absY;
    rect[2] = absX + bmp[0];
    rect[3] = absY + bmp[1];

    if (clip[2] > absX && clip[0] < rect[2] && clip[3] > absY && clip[1] < rect[3]) {
        RectClip(clip, rect, out);
        BlitFrame(PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)), bmp + 2, out, bmp[0], bmp[1], absX, absY, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, int *)(out);
    }
}
HOOK_VMETHOD(0x0043de30, CGump_0043de30, Draw);

// 0x0043df00  size 0x31
void __thiscall CGump_0043de30::MouseDown(int mx, int my)
{
    if (g_scrollChoiceGump != this)
        CGump_BringToFront(this, 0);
    dragging = 1;
    unk_6c = mx;
    unk_70 = my;
}
HOOK_VMETHOD(0x0043df00, CGump_0043de30, MouseDown);

// 0x0043df40  size 0x36
void __thiscall CGump_0043de30::MouseMove(int mx, int my)
{
    if (dragging != 0 && noDrag == 0)
        Move(mx - unk_6c, my - unk_70, 1, 0, 1);
}
HOOK_VMETHOD(0x0043df40, CGump_0043de30, MouseMove);

// 0x0043df80  size 0x12
// Enter (0x0d) calls 0x004111c0 with this still in ECX (the assembly never loads ECX).
void __thiscall CGump_0043de30::OnKey(int key)
{
    if (key == 0xd)
        reinterpret_cast<CGump_004111c0 *>(this)->Close();   // ECX is still this
}
HOOK_VMETHOD(0x0043df80, CGump_0043de30, OnKey);
