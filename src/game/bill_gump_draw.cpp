// CBillGump draw and hit-test (0x0046e970, 0x0046eac0).
#include "bill_gump.h"

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "gump_base_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_ports.h"

#define Bd_RectIntersects(a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, int *, int *)(a1, a2)
// 0x0043b590: ECX = rect, EDX = clip, stack = out, RET 4.
#define Bd_RectClip(a1, a2, a3) (GumpRectClip(a1, a2, a3))
#define Bd_BlitClipped PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, int *, int, int, int, int, void *, int, int)

// 0x0046e970  size 0x14b  vtable slot 8, RET 4
// Blits the frame bitmap clipped to `clip`, then draws the children: a child that answers true to vtable
// slot 0x98 gets the clip intersected with the whole gump, any other gets the clip intersected with the list
// rectangle (listLeft..listBottom, relative to the gump) when that overlaps `clip`, else is skipped.
void __thiscall CBillGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int r[4];
    int whole[4];   // clip against the whole gump
    int list[4];    // clip against the list rectangle
    int haveList;

    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    r[0] = sx;
    r[1] = sy;
    r[2] = sx + b6_frame_bitmap(this, 0)->w;
    r[3] = sy + b6_frame_bitmap(this, 0)->h;
    if (!Bd_RectIntersects(clip, r))   // 0x0043b540
        return;
    Bd_BlitClipped(reinterpret_cast<void *>(PORT_ADDR(&RowBlitCopyNonZero)), b6_frame_bitmap(this, 0)->px, clip, b6_frame_bitmap(this, 0)->w,
                   b6_frame_bitmap(this, 0)->h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
    Bd_RectClip(clip, r, whole);   // 0x0043b590
    r[0] = r[0] + listLeft;
    r[1] = r[1] + listTop;
    r[2] = listRight + sx;
    r[3] = listBottom + sy;
    if (Bd_RectIntersects(clip, r)) {   // 0x0043b540
        Bd_RectClip(clip, r, list);   // 0x0043b590
        haveList = 1;
    } else {
        haveList = 0;
    }
    for (CGump *c = firstChild; c != nullptr; c = c->next) {
        if (PORT_VSLOT0(c, 0x98, int)() != 0) {
            PORT_VSLOT1(c, 0x08, void, int *)(whole);
        } else if (haveList) {
            PORT_VSLOT1(c, 0x08, void, int *)(list);
        }
    }
}
HOOK_VMETHOD(0x0046e970, CBillGump, Draw);

// 0x0046eac0  size 0xb5  vtable slot 0x0c, RET 8
// 0 outside the gump; 1 when the frame bitmap pixel at (px, py) is non-zero; otherwise 1 when the point
// (converted to each child's local space, last child first) hits a child.
int __thiscall CBillGump::HitTest(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return 0;
    if (b6_frame_bitmap(this, 0)->px[py * width + px] != 0)
        return 1;
    LocalToScreen(&px, &py);   // 0x0043b4e0
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        int lx = px;
        int ly = py;
        c->ScreenToLocal(&lx, &ly);   // 0x0043b510
        if (PORT_VSLOT2(c, 0x0c, int, int, int)(lx, ly) != 0)
            return 1;
    }
    return 0;
}
HOOK_VMETHOD(0x0046eac0, CBillGump, HitTest);
