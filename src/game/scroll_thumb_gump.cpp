#include "scroll_thumb_gump.h"

#include "../runtime/ported.h"
#include "raw_field.h"
#include "gump.h"
#include "globals.h"

#define g_pDragGump GLOBAL_AS(CGump *, g_dragGump)   // gump that captures the mouse while a drag is on

// 0x0043b510: thiscall on a gump, RET 8, (&x, &y): the inverse of 0x0043b4e0 (screen -> local).
#define Orig_ScreenToLocal(self, a1, a2) (reinterpret_cast<CGump *>(self)->ScreenToLocal(a1, a2))

// 0x0041aac0  size 0xa3
// Try to grab the thumb at a screen position. The point is made local to the parent; when it lies in the box
// built from +0x9c / +0xa0 / +0xa4 the thumb becomes the drag gump, receives slot 0x10 (6, 6) and slot 0x28
// with the point local to the thumb.
// ? the meaning of +0x9c / +0xa0 / +0xa4 is not settled.
int __thiscall CScrollThumbGump::TryGrab(int sx, int sy)
{
    int lx;
    int ly;
    if (parent != nullptr) {
        lx = sx;
        ly = sy;
        Orig_ScreenToLocal(parent, &lx, &ly);
        if (lx >= RAWF(int32_t, this, 0x9c) - 1 && lx <= RAWF(int32_t, this, 0xa0) + 0xc && ly >= RAWF(int32_t, this, 0xa4) - 1 &&
            ly <= RAWF(int32_t, this, 0xa4) + 0xc) {
            g_pDragGump = this;
            PORT_VSLOT2(this, 0x10, void, int, int)(6, 6);
            Orig_ScreenToLocal(this, &sx, &sy);
            PORT_VSLOT2(this, 0x28, void, int, int)(sx, sy);
            return 1;
        }
    }
    return 0;
}
HOOK_METHOD(0x0041aac0, CScrollThumbGump, TryGrab);

// 0x0041c2f0  size 0x11
// Mouse release: the drag gump is cleared. Both arguments are ignored.
void __thiscall CDragReleaseGump::ClearDragCapture(int mx, int my)
{
    (void)mx;
    (void)my;
    g_pDragGump = nullptr;
}
HOOK_METHOD(0x0041c2f0, CDragReleaseGump, ClearDragCapture);
