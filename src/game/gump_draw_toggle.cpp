// 0x00416590  size 0xa2: the canonical Draw of the check box.
// See docs/matching.md "Gump virtual slots" (open question 6): the three Draw bodies of 0x00416590 (CToggleControl,
// CListBoxControlGump, CInputControlGump) compile to the original bytes only as the first function of a translation unit,
// and the unit must see no more declarations than it needs, so each lives in a file of its own (no vtables.h, obj.h, ...).
#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "gump_base_ops_ports.h"
#include "toggle_control.h"

void __thiscall CToggleControl::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[1] = oy;
    rect[0] = ox;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
}
HOOK_VMETHOD(0x00416590, CToggleControl, Draw);
