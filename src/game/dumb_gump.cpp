#include "dumb_gump.h"
#include "scroll_choice_gump.h"

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "gump_base_ops_ports.h"

// 0x0041dad0  size 0xb1
// Draws frame0 at the gump's absolute origin when the gump manager accepts the clip, then the children.
void __thiscall CDumbGump::DrawBlitFrame(void *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);   // absolute screen origin

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    // 0x0043b540: thiscall on the gump manager object, (clip, &rect), RET 8
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);

        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);   // child->draw(clip)
    }
}
HOOK_METHOD(0x0041dad0, CDumbGump, DrawBlitFrame);


// Slot override: same body as CDumbGump::DrawBlitFrame (0x0041dad0); the linker folds the copies.
void __thiscall CScrollChoiceGump::Draw(int *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);   // absolute screen origin

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);

        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);   // child->draw(clip)
    }
}
