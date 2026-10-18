// Factories for CMissileGump (new + constructor + link into the top-level gump list), the helper's
// deleting destructor and its bounds update. gumps.cxx, 0x00437a80..0x00437e1a.
//
// The SEH frames of the original (unwind states around operator new) are omitted.
#include "missile_gump.h"

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "crt.h"
#include "rotated_blit.h"
#include "art_record_ops_ports.h"
#include "globals.h"

// 0x00431040: ECX = angle, EDX = RECT *; rotates the rect's bounds in place.
#define MGN_RotateRectBounds(a1, a2) (RotateRectBounds(a1, (int *)(a2)))
// 0x00476130 on the tile art table (0x0068d2d8): u16 width at +4, height at +6.
#define MGN_TileArtGet(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), int)(a1)
#define MGN_TileArtTable GLOBAL_PTR(void, g_artCache_storage)

// 0x00437a80  size 0x9c
// New missile gump from world position start to world position end.
CMissileGump *__stdcall NewMissileGump_PosToPos(uint16_t art, Pos start, Pos end, uint8_t speed, uint8_t noRotate,
                                                       uint8_t flag)
{
    CMissileGump *g = new CMissileGump(start, end, art, speed, noRotate, flag);   // 0x00438150
    CGump_AddChild(g, 0, 1);                                                      // 0x0043bcc0
    return g;
}
HOOK_FUNC(0x00437a80, NewMissileGump_PosToPos);

// 0x00437b20  size 0x8e
// New missile gump from world position start to a target object.
CMissileGump *__stdcall NewMissileGump_PosToObj(uint16_t art, Pos start, CObj *target, uint8_t speed, uint8_t noRotate,
                                                       uint8_t flag)
{
    CMissileGump *g = new CMissileGump(start, target, art, speed, noRotate, flag);   // 0x00438530
    CGump_AddChild(g, 0, 1);                                                         // 0x0043bcc0
    return g;
}
HOOK_FUNC(0x00437b20, NewMissileGump_PosToObj);

// 0x00437bb0  size 0x8e
// New missile gump from a source object to world position end.
CMissileGump *__stdcall NewMissileGump_ObjToPos(uint16_t art, CObj *src, Pos end, uint8_t speed, uint8_t noRotate,
                                                       uint8_t flag)
{
    CMissileGump *g = new CMissileGump(src, end, art, speed, noRotate, flag);   // 0x00438950
    CGump_AddChild(g, 0, 1);                                                    // 0x0043bcc0
    return g;
}
HOOK_FUNC(0x00437bb0, NewMissileGump_ObjToPos);

// 0x00437c40  size 0x7f
// New missile gump from a source object to a target object.
CMissileGump *__stdcall NewMissileGump_ObjToObj(uint16_t art, CObj *src, CObj *dst, uint8_t speed, uint8_t noRotate,
                                                       uint8_t flag)
{
    CMissileGump *g = new CMissileGump(src, dst, art, speed, noRotate, flag);   // 0x00438cf0
    CGump_AddChild(g, 0, 1);                                                    // 0x0043bcc0
    return g;
}
HOOK_FUNC(0x00437c40, NewMissileGump_ObjToObj);

// 0x00437d60  size 0xf2
// Bounds of the owner's current frame: width / height from the gump art (random-frame effects) or
// the tile art (id + 0x4000); the rect is centred on the owner's x, ends at its y, is rotated by the
// owner's angle when it has one, then moved by the world scroll.
void __thiscall CMissileHelper::ComputeScreenRect()
{
    int w = 0;
    int h = 0;
    CMissileGump *m = owner;

    if (m->randomFrame != 0) {
        GumpArtSize(m->curFrame, &w, &h);                                       // 0x00433e70
    } else {
        uint8_t *art = MGN_TileArtGet(MGN_TileArtTable, m->curFrame + 0x4000);   // 0x00476130
        if (art != nullptr) {
            w = *reinterpret_cast<uint16_t *>(art + 4);
            h = *reinterpret_cast<uint16_t *>(art + 6);
        }
    }

    rectX0 = m->x - (w >> 1);
    rectX1 = rectX0 + w;
    rectY0 = m->y - h;
    rectY1 = h + rectY0;
    uint8_t angle = m->angle;
    if (angle != 0 && w > 0)
        MGN_RotateRectBounds(angle, &rectX0);                                     // 0x00431040
    rectX0 += g_scrollX;
    rectX1 += g_scrollX;
    rectY0 += g_scrollY;
    rectY1 += g_scrollY;
}
HOOK_VMETHOD(0x00437d60, CMissileHelper, ComputeScreenRect);
