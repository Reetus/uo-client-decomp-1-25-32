// Missile / effect gump pieces from gumps.cxx 0x00437e60..0x00439360: the art-draw
// helper, the world-position helper's draw handler, the gump's scalar deleting
// destructor and the "attached to an object" constructor.
//
// The SEH frame of the constructor (handler around the base ctor) is omitted.
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"
#include "missile_gump.h"
#include "crt.h"
#include "raw_field.h"
#include "art_record_ops_ports.h"
#include "art_row_blits_ports.h"
#include "rotated_blit.h"
#include "gump_art_ops_ports.h"
#include "gump.h"
#include "art_blit_ports.h"
#include "gump_art_ports.h"
#include "art_row_blits_hue_ports.h"

#define g_tileAnim GLOBAL_PTR(uint8_t, g_animData_storage)   // 0x48 bytes per art id; +0x41 frame count, +0x43 delay

// 0x00476130 on the art cache at 0x0068d2d8: record for tile art id (id + 0x4000), or 0.
#define Md_TileArt(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), uint32_t)(a1)
#define Md_ArtCache GLOBAL_PTR(void, g_artCache_storage)
// 0x004767c0: ECX = buffer, EDX = width.
#define Md_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x004767d0 (ported as RleArtWalk): ECX = row blit, EDX = art record, stack (clip, x, y), RET 0xc.
#define Md_RleWalk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, uint8_t *, int *, int, int)
// 0x00431260: ECX = sprite, EDX = rect, stack (x, y, angle, clip), RET 0x10.
#define Md_RotBlit(a1, a2, a3, a4, a5, a6) (BlitRotatedSprite((const uint8_t *)(a1), (const int *)(a2), a3, a4, a5, (const int *)(a6)))
// 0x00433e70: ECX = art id, EDX = &w, stack &h, RET 4.
#define Md_GumpArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))
// 0x00434060: ECX = row blit, EDX = art id, stack (clip, x, y, pixels, w, h), RET 0x18.
#define Md_DrawArtClipped PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, void *, int, int *, int, int, void *, int, int)
// 0x0043c3d0: CGump base constructor.

// 0x00437e60  size 0xfc
// Draw tile art (artId + 0x4000) at rect[0], rect[1] minus the scroll, rotated by `angle`
// when it is nonzero. ECX = art id, DL = angle, stack (rect, clip), RET 8.
// ? The original pushes the angle as a dword whose upper bytes it never wrote; the callee
//   masks it with 0xff, so a byte is faithful.
void __fastcall MissileDrawTileArt(uint16_t artId, uint8_t angle, int *rect, int *clip)
{
    int r[4];
    r[0] = rect[0];
    r[1] = rect[1];
    r[2] = rect[2];
    r[3] = rect[3];

    uint8_t *art = Md_TileArt(Md_ArtCache, static_cast<uint32_t>(artId) + 0x4000);
    if (art == nullptr)
        return;

    int x = rect[0];
    int y = rect[1];
    int w = *reinterpret_cast<uint16_t *>(art + 4);
    int h = *reinterpret_cast<uint16_t *>(art + 6);
    int hx = (w >> 1) - g_scrollX;
    x = x - g_scrollX;
    y = y - g_scrollY;
    r[0] += hx - x;
    r[2] += hx - x;
    int hy = (h >> 1) - g_scrollY;
    r[1] += hy - y;
    r[3] += hy - y;

    if (angle != 0) {
        Md_RotBlit(art, r, x, y, angle, clip);
        return;
    }
    Md_SetTarget(g_backBuffer, 0x280);
    Md_RleWalk(PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)), art, clip, x, y);
}
HOOK_FUNC(0x00437e60, MissileDrawTileArt);

// 0x00437f60  size 0x1d6
// Draw handler of the world-position helper (vtable 0x0049ebd0 slot +0x08): draws the owner
// gump's current frame (gump art when owner+0xfc is set, tile art otherwise) at the helper's
// screen rect. One stack argument (clip), RET 4.
// ? the vertical offset uses the full height, the horizontal one half the width, as the
//   assembly does.
void __thiscall CMissileHelper::Draw(int *clip)
{
    struct Box4 { int v[4]; };
    Box4 box = *reinterpret_cast<Box4 *>(&rectX0);
    int *r = box.v;

    CMissileGump *o = owner;
    uint8_t *art;
    int w;
    int h;
    if (o->randomFrame != 0) {
        art = nullptr;
        Md_GumpArtSize(o->curFrame, &w, &h);   // 0x00433e70
        if (w == 0)
            return;
    } else {
        art = Md_TileArt(Md_ArtCache, static_cast<uint32_t>(o->curFrame) + 0x4000);
        if (art == nullptr)
            return;
        w = *reinterpret_cast<uint16_t *>(art + 4);
        h = *reinterpret_cast<uint16_t *>(art + 6);
    }

    int sx = RAWF(int, this, 4) - g_scrollX;
    int sy = RAWF(int, this, 8) - g_scrollY;
    o = owner;
    r[0] += (w >> 1) - g_scrollX - o->x;
    r[2] += (w >> 1) - g_scrollX - o->x;
    r[1] += h - g_scrollY - o->y;
    r[3] += h - g_scrollY - o->y;

    if (o->randomFrame != 0) {
        Md_DrawArtClipped(PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)), o->curFrame, clip, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x00434060
        return;
    }
    if (o->angle != 0) {
        Md_RotBlit(art, r, sx, sy, o->angle, clip);
        return;
    }
    Md_SetTarget(g_backBuffer, 0x280);
    if (g_player != nullptr && PORT_VSLOT0(g_player, 0x8c, int)() != 0) {
        g_hueOverride = 0x3b1;
        Md_RleWalk(PTR_AT(void, PORT_ADDR(&RowBlitLookupAll)), art, clip, sx, sy);
        return;
    }
    Md_RleWalk(PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)), art, clip, sx, sy);
}
HOOK_VMETHOD(0x00437f60, CMissileHelper, Draw);

HOOK_VTABLE(0x0049ec28, CMissileGump);
HOOK_SDD(0x00438500, CMissileGump);

// 0x00439360  size 0x187
// Effect attached to an object: art plays on the object, and the gump is appended to the
// object's effect list at obj+0x1c0 (linked through nextEffect). SEH frame omitted.
CMissileGump::CMissileGump(CObj *obj, uint16_t art, uint8_t speed, uint8_t life, uint8_t flagA, uint8_t flagB)
{
    lifetime = life;
    frameDelay = static_cast<uint8_t>(0x0b - speed);
    sequence = nullptr;
    angle = 0;
    delayCounter = 0;
    stepSpeed = 0;

    unsigned a = static_cast<unsigned>(art) * 0x48;
    if (g_tileAnim[a + 0x41] != 0) {
        tiledataAnim = 1;
        frameDelay = g_tileAnim[a + 0x43];
        firstFrame = art;
        lastFrame = 0;
        curFrame = static_cast<uint16_t>(static_cast<int16_t>(static_cast<int8_t>(g_tileAnim[a])) + art);
    } else {
        tiledataAnim = 0;
        firstFrame = art;
        curFrame = art;
        lastFrame = static_cast<uint16_t>(g_tileData[static_cast<unsigned>(art) * 0x28 + 5] + art - 1);
    }
    randomFrame = 0;
    backward = (flagA != 0);
    pingPong = (flagB != 0);
    unk_f4 = 0;
    spawnImpact = 0;
    spin = 0;
    attached = 1;
    destX = 0;
    destY = 0;
    destZ = 0;
    target = obj;
    nextEffect = nullptr;
    name = const_cast<char *>("missile gump");   // 0x004a8364
    dirty = 0;
    unk_50 = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    helper = nullptr;

    CMissileGump *list = RAWF(CMissileGump *, obj, 0x1c0);
    if (list == nullptr) {
        RAWF(CMissileGump *, obj, 0x1c0) = this;
    } else {
        CMissileGump *n = list->nextEffect;
        while (n != nullptr) {
            list = n;
            n = list->nextEffect;
        }
        list->nextEffect = this;
    }
}
HOOK_CTOR(0x00439360, CMissileGump, (CObj *, uint16_t, uint8_t, uint8_t, uint8_t, uint8_t));

// 0x00439e80  size 0x9
// Vtable slot 0x0c (hit test), RET 8: the effect gump is never hit.
int __thiscall CMissileGump::HitTest(int, int)
{
    return 0;
}
HOOK_VMETHOD(0x00439e80, CMissileGump, HitTest);

// Empty overrides of slots 0x04 and 0x08 (the table held the empty Stub_00401260, RET 4; folded by the linker).
void __thiscall CMissileGump::PreDraw(int *)
{
}

void __thiscall CMissileGump::Draw(int *)
{
}
