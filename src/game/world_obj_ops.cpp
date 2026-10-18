// World object virtual methods of 0x0042ad20 .. 0x0042c730. The SEH frames of the original (try levels around
// the `operator new` of the slice objects) are omitted. (Porting pass 2, batch 10.)
#include "world_obj_ops.h"

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "class_0049c690.h"
#include "obj.h"
#include "globals.h"
#include "crt.h"
#include "world_renderer.h"
#include "light_tables_ports.h"
#include "client_misc_ops_ports.h"
#include "map_gump_ops_ports.h"
#include "circle_trans.h"
#include "sprite_draw_ports.h"
#include "dirty_rects_ports.h"
#include "art_blit_ports.h"
#include "art_row_blits_ports.h"
#include "art_row_blits_hue_ports.h"
#include "vtables.h"

#define g_worldBuckets GLOBAL_PTR(CWorldObj *, g_tileBuckets)    // 64 x 64 bucket heads
#define g_circleRect GLOBAL_PTR(void, g_circleTrans_storage)             // CCircleTrans object; its bounding rect is at +0

// 0x0044f8b0: invalidate a screen rect (ECX = &{x0, y0, x1, y1}).
#define Screen_Invalidate(o) PORT_THISCALL0(&DirtyRects_Invalidate, void, (o))()
// 0x0044f140: invalidate an item (ECX = item).
#define Item_Invalidate(o) PORT_THISCALL0(&InvalidateLightRect, void, (o))()
// 0x00478cd0: a stub that returns 0; plain RET, no arguments.
#define IsOptionSet() (Misc_Return0())
// 0x00445280: ECX = map object (not read), stack (a, b) two {int16 x, int16 y} points, RET 8.
#define Map_PointDistance(o, a1, a2) PORT_THISCALL2(&MapDistancePoints, int, (o), const int16_t *, const int16_t *)(a1, a2)
// 0x0044f510 / 0x0044f3c0: draw a land tile; ECX = 0, EDX = x, stack (y, clip), RET 8.
#define Tile_DrawGhost(a1, a2, a3, a4) (DrawLightFootprintRemap((unsigned)(a1), a2, a3, a4))
#define Tile_DrawNormal(a1, a2, a3, a4) (DrawLightFootprint((unsigned)(a1), a2, a3, a4))
// 0x004244e0 (circle_trans.cpp): non-zero when the object is drawn through the see-through circle.
#define Obj_OverlapsCircle(self) (reinterpret_cast<CWorldObj_004244e0 *>(self)->OverlapsCircle())
// 0x004246e0 (sprite_draw.cpp): ECX = row blitter, EDX = art record, stack (clip, x, y), RET 0xc.
#define Art_DrawClipped(a1, a2, a3, a4, a5) (DrawRleSpriteClipped((int)(a1), a2, a3, a4, a5))
// 0x004767d0 (art_blit.cpp): same arguments, clipped walk.
#define Art_WalkClipped PORT_FUNC5(&RleArtWalk, void, __fastcall, uintptr_t, uint8_t *, int *, int, int)
// 0x00476990: ECX = row blitter, EDX = art record, stack (x, y), RET 8.
#define Art_DrawUnclipped PORT_FUNC4(&RleArtWalkUnclipped, void, __fastcall, uintptr_t, uint8_t *, int, int)

struct ObjRect4 {
    int x0, y0, x1, y1;
};

static inline uint16_t objGraphic(CWorldObj *o)
{
    return RAW_AT(uint16_t, o, 0x38);
}
static inline int objTileOffset(CWorldObj *o)
{
    return RAW_AT(int8_t, o, 0x3a);
}

// 0x0042ae40  size 0x9c
void __thiscall CWorldObj::RemoveFromWorld()
{
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
    if (next != nullptr)
        next->prev = prev;
    if (prev != nullptr) {
        prev->next = next;
    } else {
        unsigned by = static_cast<uint8_t>(y);
        unsigned bx = static_cast<uint8_t>(x);
        g_worldBuckets[((by & 0x3f) << 6) + (bx & 0x3f)] = next;
    }
    inWorld = 0;
    prev = nullptr;
    next = nullptr;
}
HOOK_VMETHOD(0x0042ae40, CWorldObj, RemoveFromWorld);

// 0x0042aee0  size 0xc3
void __thiscall CObjVt49f9e0::RemoveFromWorld()
{
    for (;;) {
        void *child = RAW_AT(void *, this, 0xb0);
        if (child == nullptr)
            break;
        PORT_VSLOT0(child, 0x04, void)();       // the child unlinks itself from the list
    }
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
    if (next != nullptr)
        next->prev = prev;
    if (prev != nullptr) {
        prev->next = next;
    } else {
        int skew = RAW_AT(int32_t, this, 0xb8);
        unsigned by = static_cast<uint16_t>(y) - skew;
        unsigned bx = static_cast<uint16_t>(x) + skew;
        g_worldBuckets[((by & 0x3f) << 6) + (bx & 0x3f)] = next;
    }
    inWorld = 0;
    prev = nullptr;
    next = nullptr;
}
HOOK_VMETHOD(0x0042aee0, CObjVt49f9e0, RemoveFromWorld);

// 0x0042afb0  size 0xee
void __thiscall CMobile::RemoveFromWorld()
{
    if (inWorld == 0)
        return;
    for (;;) {
        void *child = RAW_AT(void *, this, 0xb0);
        if (child == nullptr)
            break;
        PORT_VSLOT0(child, 0x04, void)();
    }
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
    if (next != nullptr)
        next->prev = prev;
    if (prev != nullptr) {
        prev->next = next;
    } else {
        int bx = x;
        int by = y;
        if (RAW_AT(int32_t, this, 0x13c) != 0) {
            unsigned facing = RAW_AT(uint8_t, this, 0x6e);
            switch (facing) {
            case 0:
            case 1:
                by++;
                break;
            case 5:
            case 6:
                bx++;
                break;
            case 7:
                bx++;
                by++;
                break;
            default:
                break;
            }
        }
        int skew = RAW_AT(int32_t, this, 0xb8);
        unsigned col = (bx + skew) & 0x3f;
        g_worldBuckets[(((by - skew) & 0x3f) << 6) + col] = next;
    }
    inWorld = 0;
    prev = nullptr;
    next = nullptr;
}
HOOK_VMETHOD(0x0042afb0, CMobile, RemoveFromWorld);

// 0x0042b0c0  size 0xef
void __thiscall CPlayer::RemoveFromWorld()
{
    if (inWorld == 0)
        return;
    for (;;) {
        void *child = RAW_AT(void *, this, 0xb0);
        if (child == nullptr)
            break;
        PORT_VSLOT0(child, 0x04, void)();
    }
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
    if (next != nullptr)
        next->prev = prev;
    if (prev != nullptr) {
        prev->next = next;
    } else {
        int bx = x;
        int by = y;
        if (g_altFacingFlag != 0) {
            unsigned facing = RAW_AT(uint8_t, this, 0x6e) - 1u;
            switch (facing) {
            case 0:
            case 1:
                bx++;
                break;
            case 2:
                bx++;
                by++;
                break;
            case 3:
            case 4:
                by++;
                break;
            default:
                break;
            }
        }
        int skew = RAW_AT(int32_t, this, 0xb8);
        g_worldBuckets[(((by - skew) & 0x3f) << 6) + ((bx + skew) & 0x3f)] = next;
    }
    inWorld = 0;
    prev = nullptr;
    next = nullptr;
}
HOOK_VMETHOD(0x0042b0c0, CPlayer, RemoveFromWorld);

// 0x0042b1d0  size 0xf9
void __thiscall CWorldObj::InsertIntoBucket(int *px, int *py)
{
    inWorld = 1;
    PORT_VSLOT0(this, 0x0c, void)();             // ComputeScreenRect
    *px = (*px + RAW_AT(int32_t, this, 0xb8)) & 0x3f;
    *py = (*py - RAW_AT(int32_t, this, 0xb8)) & 0x3f;
    CWorldObj *before = nullptr;
    for (CWorldObj *cur = g_worldBuckets[(*py << 6) + *px]; cur != nullptr && cur->depth <= depth; cur = cur->next)
        before = cur;
    if (before != nullptr) {
        prev = before;
        next = before->next;
        before->next = this;
        if (next != nullptr)
            next->prev = this;
    } else {
        CWorldObj *h = g_worldBuckets[(*py << 6) + *px];
        next = h;
        if (h != nullptr)
            h->prev = this;
        g_worldBuckets[(*py << 6) + *px] = this;
        prev = nullptr;
    }
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
}
HOOK_METHOD(0x0042b1d0, CWorldObj, InsertIntoBucket);

// 0x0042b2d0  size 0x102
void __thiscall CWorldObj::AddToWorld()
{
    int ex = x;
    int ey = y;
    depth = z;
    sumXY = ex + ey;
    key28 = g_scrollBase - ey + ex;
    if (PORT_VSLOT0(this, 0x30, int)() != 0) {
        int skew = RAW_AT(int32_t, this, 0xb8);
        ex = (ex + skew) & 0x3f;
        ey = (ey - skew) & 0x3f;
    }
    CWorldObj *before = nullptr;
    for (CWorldObj *cur = g_worldBuckets[((ey & 0x3f) << 6) + (ex & 0x3f)]; cur != nullptr && cur->depth <= depth; cur = cur->next)
        before = cur;
    if (before != nullptr) {
        prev = before;
        next = before->next;
        before->next = this;
        if (next != nullptr)
            next->prev = this;
    } else {
        CWorldObj *h = g_worldBuckets[((ey & 0x3f) << 6) + (ex & 0x3f)];
        next = h;
        if (h != nullptr)
            h->prev = this;
        g_worldBuckets[((ey & 0x3f) << 6) + (ex & 0x3f)] = this;
        prev = nullptr;
    }
    inWorld = 1;
    PORT_VSLOT0(this, 0x0c, void)();
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
}
HOOK_VMETHOD(0x0042b2d0, CWorldObj, AddToWorld);

// 0x0042b3e0  size 0x105
void __thiscall CWorldObjLow::AddToWorld()
{
    int ey = y;
    int ex = x;
    depth = z - 2;
    sumXY = ex + ey;
    key28 = g_scrollBase - ey + ex;
    if (PORT_VSLOT0(this, 0x30, int)() != 0) {
        int skew = RAW_AT(int32_t, this, 0xb8);
        ex = (ex + skew) & 0x3f;
        ey = (ey - skew) & 0x3f;
    }
    CWorldObj *before = nullptr;
    for (CWorldObj *cur = g_worldBuckets[((ey & 0x3f) << 6) + (ex & 0x3f)]; cur != nullptr && cur->depth <= depth; cur = cur->next)
        before = cur;
    if (before != nullptr) {
        prev = before;
        next = before->next;
        before->next = this;
        if (next != nullptr)
            next->prev = this;
    } else {
        CWorldObj *h = g_worldBuckets[((ey & 0x3f) << 6) + (ex & 0x3f)];
        next = h;
        if (h != nullptr)
            h->prev = this;
        g_worldBuckets[((ey & 0x3f) << 6) + (ex & 0x3f)] = this;
        prev = nullptr;
    }
    inWorld = 1;
    PORT_VSLOT0(this, 0x0c, void)();
    if (PORT_VSLOT0(this, 0x3c, int)() == 0 && inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
}
HOOK_VMETHOD(0x0042b3e0, CObjVt49f730, AddToWorld);

// 0x0042b620  size 0x1f0
// Inserts the object itself, then one 0x54-byte slice object (class 0x0049c690, constructor 0x004065b0) per 0x2c
// pixels of its screen height, each moved one cell further along the diagonal, and links every slice in its bucket.
// The slice code is the same in 0x0042b810 and 0x0042ba80; the original has three copies.
void __thiscall CObjVt49f9e0::AddToWorld()
{
    int ey = y;
    depth = z + 1;
    int ex = x;
    sumXY = ey + ex;
    key28 = g_scrollBase + ex - ey;
    InsertIntoBucket(&ex, &ey);

    int top = rectY1;
    int base = RAW_AT(int32_t, this, 0xb4);
    if (top > base) {
        int step = 1;
        int sliceBase = base;
        if (base < top) {
            int lim = base + 0x2c;
            for (;;) {
                new CObj0049c690(static_cast<int16_t>(step + ex), static_cast<int16_t>(step + ey),
                              sliceBase, lim <= rectY1 ? lim : rectY1, reinterpret_cast<CObj0049c690 *>(this));
                CWorldObj *child = RAW_AT(CWorldObj *, this, 0xb0);
                int sy = (step + ey) & 0x3f;
                int sx = (step + ex) & 0x3f;
                if (PORT_VSLOT0(child, 0x30, int)() != 0) {
                    int skew = RAW_AT(int32_t, child, 0xb8);
                    sx = (sx + skew) & 0x3f;
                    sy = (sy - skew) & 0x3f;
                }
                int idx = ((sy & 0x3f) << 6) + (sx & 0x3f);
                CWorldObj *head = g_worldBuckets[idx];
                CWorldObj *before = nullptr;
                CWorldObj *cur = head;
                if (cur != nullptr) {
                    do {
                        if (cur->depth > child->depth)
                            break;
                        before = cur;
                        cur = cur->next;
                    } while (cur != nullptr);
                }
                if (before != nullptr) {
                    child->prev = before;
                    child->next = before->next;
                    before->next = child;
                    if (child->next != nullptr)
                        child->next->prev = child;
                } else {
                    child->next = head;
                    if (head != nullptr)
                        head->prev = child;
                    g_worldBuckets[idx] = child;
                    child->prev = nullptr;
                }
                child->inWorld = 1;
                PORT_VSLOT0(child, 0x0c, void)();
                if (PORT_VSLOT0(child, 0x3c, int)() == 0 && child->inWorld != 0) {
                    ObjRect4 r;
                    r.y0 = child->rectY0 - g_scrollY;
                    r.y1 = child->rectY1 - g_scrollY;
                    r.x0 = child->rectX0 - g_scrollX;
                    r.x1 = child->rectX1 - g_scrollX;
                    Screen_Invalidate(&r);
                }
                sliceBase += 0x2c;
                step++;
                lim += 0x2c;
                if (!(sliceBase < rectY1))
                    break;
            }
        }
    }
}
HOOK_VMETHOD(0x0042b620, CObjVt49f9e0, AddToWorld);

// 0x0042b810  size 0x24f
void __thiscall CMobile::AddToWorld()
{
    if (inWorld != 0)
        PORT_VSLOT0(this, 0x14, void)();
    int ez = z;
    int ey = y;
    depth = ez;
    int ex = x;
    sumXY = ex + ey;
    key28 = g_scrollBase + ex - ey;
    if (PORT_VSLOT0(this, 0x6c, int)() > 0)
        depth = depth + 1;
    int lx = x;
    int ly = y;
    if (RAW_AT(int32_t, this, 0x13c) != 0) {
        unsigned facing = RAW_AT(uint8_t, this, 0x6e);
        switch (facing) {
        case 0:
        case 1:
            ly++;
            break;
        case 5:
        case 6:
            lx++;
            break;
        case 7:
            lx++;
            ly++;
            break;
        default:
            break;
        }
    }
    InsertIntoBucket(&lx, &ly);
    if (objGraphic(this) < 0x190) {
        int top = rectY1;
        int base = RAW_AT(int32_t, this, 0xb4);
        if (top > base) {
            int step = 1;
            int sliceBase = base;
            if (base < top) {
                int lim = base + 0x2c;
                for (;;) {
                    new CObj0049c690(static_cast<int16_t>(step + lx), static_cast<int16_t>(step + ly),
                                  sliceBase, lim <= rectY1 ? lim : rectY1, reinterpret_cast<CObj0049c690 *>(this));
                    CWorldObj *child = RAW_AT(CWorldObj *, this, 0xb0);
                    int sy = (step + ly) & 0x3f;
                    int sx = (step + lx) & 0x3f;
                    if (PORT_VSLOT0(child, 0x30, int)() != 0) {
                        int skew = RAW_AT(int32_t, child, 0xb8);
                        sx = (sx + skew) & 0x3f;
                        sy = (sy - skew) & 0x3f;
                    }
                    int idx = ((sy & 0x3f) << 6) + (sx & 0x3f);
                    CWorldObj *head = g_worldBuckets[idx];
                    CWorldObj *before = nullptr;
                    CWorldObj *cur = head;
                    if (cur != nullptr) {
                        do {
                            if (cur->depth > child->depth)
                                break;
                            before = cur;
                            cur = cur->next;
                        } while (cur != nullptr);
                    }
                    if (before != nullptr) {
                        child->prev = before;
                        child->next = before->next;
                        before->next = child;
                        if (child->next != nullptr)
                            child->next->prev = child;
                    } else {
                        child->next = head;
                        if (head != nullptr)
                            head->prev = child;
                        g_worldBuckets[idx] = child;
                        child->prev = nullptr;
                    }
                    child->inWorld = 1;
                    PORT_VSLOT0(child, 0x0c, void)();
                    if (PORT_VSLOT0(child, 0x3c, int)() == 0 && child->inWorld != 0) {
                        ObjRect4 r;
                        r.y0 = child->rectY0 - g_scrollY;
                        r.y1 = child->rectY1 - g_scrollY;
                        r.x0 = child->rectX0 - g_scrollX;
                        r.x1 = child->rectX1 - g_scrollX;
                        Screen_Invalidate(&r);
                    }
                    sliceBase += 0x2c;
                    step++;
                    lim += 0x2c;
                    if (!(sliceBase < rectY1))
                        break;
                }
            }
        }
    }
    PORT_VSLOT0(this, 0x90, void)();
}
HOOK_VMETHOD(0x0042b810, CMobile, AddToWorld);

// 0x0042ba80  size 0x24f
void __thiscall CPlayer::AddToWorld()
{
    if (inWorld != 0)
        PORT_VSLOT0(this, 0x14, void)();
    int ez = z;
    int ex = x;
    depth = ez;
    int ey = y;
    sumXY = ey + ex;
    key28 = g_scrollBase + ex - ey;
    if (PORT_VSLOT0(this, 0x6c, int)() > 0)
        depth = depth + 1;
    int lx = x;
    int ly = y;
    if (g_altFacingFlag != 0) {
        unsigned facing = RAW_AT(uint8_t, this, 0x6e) - 1u;
        switch (facing) {
        case 0:
        case 1:
            lx++;
            break;
        case 2:
            lx++;
            ly++;
            break;
        case 3:
        case 4:
            ly++;
            break;
        default:
            break;
        }
    }
    InsertIntoBucket(&lx, &ly);
    if (objGraphic(this) < 0x190) {
        int top = rectY1;
        int base = RAW_AT(int32_t, this, 0xb4);
        if (top > base) {
            int step = 1;
            int sliceBase = base;
            if (base < top) {
                int lim = base + 0x2c;
                for (;;) {
                    new CObj0049c690(static_cast<int16_t>(step + lx), static_cast<int16_t>(step + ly),
                                  sliceBase, lim <= rectY1 ? lim : rectY1, reinterpret_cast<CObj0049c690 *>(this));
                    CWorldObj *child = RAW_AT(CWorldObj *, this, 0xb0);
                    int sy = (step + ly) & 0x3f;
                    int sx = (step + lx) & 0x3f;
                    if (PORT_VSLOT0(child, 0x30, int)() != 0) {
                        int skew = RAW_AT(int32_t, child, 0xb8);
                        sx = (sx + skew) & 0x3f;
                        sy = (sy - skew) & 0x3f;
                    }
                    int idx = ((sy & 0x3f) << 6) + (sx & 0x3f);
                    CWorldObj *head = g_worldBuckets[idx];
                    CWorldObj *before = nullptr;
                    CWorldObj *cur = head;
                    if (cur != nullptr) {
                        do {
                            if (cur->depth > child->depth)
                                break;
                            before = cur;
                            cur = cur->next;
                        } while (cur != nullptr);
                    }
                    if (before != nullptr) {
                        child->prev = before;
                        child->next = before->next;
                        before->next = child;
                        if (child->next != nullptr)
                            child->next->prev = child;
                    } else {
                        child->next = head;
                        if (head != nullptr)
                            head->prev = child;
                        g_worldBuckets[idx] = child;
                        child->prev = nullptr;
                    }
                    child->inWorld = 1;
                    PORT_VSLOT0(child, 0x0c, void)();
                    if (PORT_VSLOT0(child, 0x3c, int)() == 0 && child->inWorld != 0) {
                        ObjRect4 r;
                        r.y0 = child->rectY0 - g_scrollY;
                        r.y1 = child->rectY1 - g_scrollY;
                        r.x0 = child->rectX0 - g_scrollX;
                        r.x1 = child->rectX1 - g_scrollX;
                        Screen_Invalidate(&r);
                    }
                    sliceBase += 0x2c;
                    step++;
                    lim += 0x2c;
                    if (!(sliceBase < rectY1))
                        break;
                }
            }
        }
    }
    PORT_VSLOT0(this, 0x90, void)();
}
HOOK_VMETHOD(0x0042ba80, CPlayer, AddToWorld);

// 0x0042bcf0  size 0x43
void __thiscall CWorldObj::Invalidate()
{
    if (inWorld != 0) {
        ObjRect4 r;
        r.y0 = rectY0 - g_scrollY;
        r.y1 = rectY1 - g_scrollY;
        r.x0 = rectX0 - g_scrollX;
        r.x1 = rectX1 - g_scrollX;
        Screen_Invalidate(&r);
    }
}
HOOK_METHOD(0x0042bcf0, CWorldObj, Invalidate);

// 0x0042bd40  size 0x4e
void __thiscall CWorldObj::InvalidateEquipment()
{
    if (inWorld == 0)
        return;
    for (int i = 0; i < 0x1a; i++) {
        void *item = RAW_AT(void *, this, 0xc4 + static_cast<size_t>(i) * 4);
        if (item != nullptr && RAW_AT(uint8_t, item, 0x6d) != 0)
            Item_Invalidate(item);
    }
    if (g_ccVal220 != 0 && static_cast<void *>(this) == static_cast<void *>(g_player))
        Screen_Invalidate(g_circleRect);
}
HOOK_METHOD(0x0042bd40, CWorldObj, InvalidateEquipment);

// 0x0042c130  size 0x272
int __thiscall CObj::IsHidden()
{
    if (inWorld == 0 || g_player == nullptr)
        return 1;
    unsigned fl = PORT_VSLOT0(this, 0x40, unsigned)();
    if (g_worldRenderer.clearFirst != 0) {
        for (CWorldObj *s = prev; s != nullptr; s = s->prev) {
            if (PORT_VSLOT0(s, 0x1c, int)() != 0 && s->z >= g_worldRenderer.hideZ)
                return 1;
        }
        if (g_worldRenderer.clearFirst != 0 && z >= g_worldRenderer.hideZ)
            return 1;
    }
    int limit = g_worldRenderer.roofLimit;
    if (((fl & 0x10000000) != 0 && limit != 0x7f) || z >= limit || ((fl & 0x10000) != 0 && IsOptionSet() == 0))
        return 1;
    if ((fl & 0x20000) == 0)
        return 0;
    if (g_fadingPass == 0)
        return RAW_AT(int32_t, this, 0x70);

    RAW_AT(int32_t, this, 0x70) = 0;
    int k = 0;
    int lo = g_foliageRanges[0];
    if (lo != 0) {
        int g = objGraphic(this);
        int32_t *e = g_foliageRanges;
        for (;;) {
            if (g >= lo && g <= e[1])
                break;
            lo = e[2];
            e += 2;
            k += 2;
            if (lo == 0)
                break;
        }
    }
    CWorldObj *pl = reinterpret_cast<CWorldObj *>(g_player);
    if (g_foliageRanges[k] != 0) {
        for (CWorldObj *e = GLOBAL_AS(CWorldObj *, g_list20000Head); e != nullptr; e = RAW_AT(CWorldObj *, e, 0x58)) {
            if (e->inWorld == 0)
                continue;
            int eg = objGraphic(e);
            if (eg < g_foliageRanges[k])
                continue;
            if (eg > g_foliageRanges[k + 1])
                continue;
            if (!(pl->rectX1 > e->rectX0))
                continue;
            if (!(pl->rectX0 < e->rectX1))
                continue;
            if (!(pl->rectY1 > e->rectY0))
                continue;
            if (!(pl->rectY0 < e->rectY1))
                continue;
            int d = Map_PointDistance(g_mapObject, &e->x, &x);
            int diff = static_cast<int>(objGraphic(this)) - static_cast<int>(objGraphic(e));
            if (diff < 0)
                diff = -diff;
            if (d != diff)
                continue;
            int esum = e->y + e->x;
            pl = reinterpret_cast<CWorldObj *>(g_player);
            if (pl->x + pl->y >= esum)
                continue;
            if (esum == y + x) {
                RAW_AT(int32_t, this, 0x70) = 1;
                return 1;
            }
        }
        return 0;
    }
    if (pl->rectX1 > rectX0 && pl->rectX0 < rectX1 && pl->rectY1 > rectY0 && pl->rectY0 < rectY1 &&
        pl->x + pl->y < y + x) {
        RAW_AT(int32_t, this, 0x70) = 1;
        return 1;
    }
    return 0;
}
HOOK_VMETHOD(0x0042c130, CObj, IsHidden);

// 0x0042c3b0  size 0x2c4
// 1 when each of the cells (x, y), (x + 1, y + 1), (x + 1, y), (x, y + 1) and (x + 2, y + 2) holds an object
// at least 0x14 above this one that is not excluded by slots 0x24 / 0x1c / 0x38 / 0x3c and has a tiledata flag
// of 0x10000200. The original has the five cell searches written out one after the other.
int __thiscall CMobile::IsHidden()
{
    if (inWorld == 0)
        return 1;
    if (z >= g_worldRenderer.roofLimit)
        return 1;
    if (g_worldRenderer.clearFirst != 0) {
        for (CWorldObj *s = prev; s != nullptr; s = s->prev) {
            if (PORT_VSLOT0(s, 0x1c, int)() != 0 && s->z > g_worldRenderer.hideZ)
                return 1;
        }
    }
    int ey = y;
    int ex = x;
    int zmin = z + 0x14;
    static const int dxs[5] = {0, 1, 1, 0, 2};
    static const int dys[5] = {0, 1, 0, 1, 2};
    for (int i = 0; i < 5; i++) {
        CWorldObj *o = g_worldBuckets[(((ey + dys[i]) & 0x3f) << 6) + ((ex + dxs[i]) & 0x3f)];
        bool found = false;
        for (; o != nullptr; o = o->next) {
            if (o->z < zmin)
                continue;
            if (PORT_VSLOT0(o, 0x24, int)() != 0)
                continue;
            if (PORT_VSLOT0(o, 0x1c, int)() != 0)
                continue;
            if (PORT_VSLOT0(o, 0x38, int)() != 0)
                continue;
            if (PORT_VSLOT0(o, 0x3c, int)() != 0)
                continue;
            if ((PORT_VSLOT0(o, 0x40, unsigned)() & 0x10000200) != 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return 0;
    }
    return 1;
}
HOOK_VMETHOD(0x0042c3b0, CMobile, IsHidden);

// 0x0042c680  size 0xb
int __thiscall CWorldObj::IsHidden()
{
    return inWorld == 0;
}
HOOK_VMETHOD(0x0042c680, CWorldObj, IsHidden);

// Slot 0x3c of the player class: the same body as the root's (the linker folds the two).
int __thiscall CPlayer::IsHidden()
{
    return inWorld == 0;
}

// 0x0042c690  size 0x9b
void __thiscall CObj::ComputeScreenRect()
{
    uint16_t *art = PORT_VSLOT0(this, 0x74, uint16_t *)();
    if (art != nullptr) {
        int outline = PORT_VSLOT0(this, 0x78, int)();
        int w = art[2];
        int h = art[3];
        if (outline != 0) {
            w += 5;
            h += 5;
        }
        rectY1 = sumXY * 22 + 0x2b - (z << 2);
        rectY0 = rectY1 - h;
        rectX0 = key28 * 22 - (w >> 1);
        rectX1 = w + rectX0;
        return;
    }
    rectY0 = sumXY * 22;
    rectY0 -= (z << 2);
    rectY1 = rectY0 + 0x2c;
    rectX0 = key28 * 22;
    rectX1 = rectX0 + 0x2c;
}
HOOK_VMETHOD(0x0042c690, CObj, ComputeScreenRect);

// 0x0042c730  size 0x363
void __thiscall CObj::Draw(int *clip)
{
    int ghost = 0;                  // edi
    int restoreHue = 0;             // [ebp-0x14]
    unsigned graphic = objGraphic(this);
    if (graphic == 1 || (graphic > 0x2197 && graphic <= 0x21a2)) {
        if (g_showHidden == 0)
            return;
    }
    if (PORT_VSLOT0(this, 0x2c, int)() != 0 && (RAW_AT(uint8_t, this, 0x98) & 0x80) != 0) {
        if (IsOptionSet() == 0) {
            if (g_player == nullptr)
                return;
            if (RAW_AT(uint16_t, g_player, 0x38) != 0x3db)
                return;
        }
        ghost = 1;
    }
    uint8_t *art = PORT_VSLOT0(this, 0x74, uint8_t *)();
    int outline = PORT_VSLOT0(this, 0x78, int)();
    if (ghost == 0)
        ghost = PORT_VSLOT0(g_player, 0x8c, int)();
    if ((*reinterpret_cast<uint32_t *>(g_tileData + (objTileOffset(this) + objGraphic(this)) * 0x28) & 0x10000) != 0) {
        if (IsOptionSet() == 0)
            return;
    }
    if (art == nullptr) {
        if (ghost != 0) {
            Tile_DrawGhost(0, rectX0 - g_scrollX, rectY0 - g_scrollY, clip);
            return;
        }
        Tile_DrawNormal(0, rectX0 - g_scrollX, rectY0 - g_scrollY, clip);
        return;
    }
    unsigned hue = RAW_AT(uint16_t, this, 0x3e);
    if (hue > 0xea60)
        return;
    int needClip = 0;
    if (clip[0] > rectX0 - g_scrollX || clip[2] < rectX1 - g_scrollX || clip[1] > rectY0 - g_scrollY
        || clip[3] < rectY1 - g_scrollY || outline != 0)
        needClip = 1;
    if (ghost != 0) {
        RAW_AT(uint16_t, this, 0x3e) = 0x3b2;
        restoreHue = static_cast<int>(hue) + 0x1f4;
    }
    if (static_cast<void *>(this) == RAW_AT(void *, g_screenGump, 0xd8))
        g_hueOverride = 0x34;
    else
        g_hueOverride = static_cast<int>(RAW_AT(uint16_t, this, 0x3e)) - 1;
    unsigned mode = *reinterpret_cast<uint32_t *>(g_tileData + (objTileOffset(this) + objGraphic(this)) * 0x28) & 0x60008;
    if (ghost != 0)
        mode = 0;
    int sx = rectX0 - g_scrollX;
    int sy = rectY0 - g_scrollY;
    if (RAW_AT(uint16_t, this, 0x3e) > 0 || static_cast<void *>(this) == RAW_AT(void *, g_screenGump, 0xd8))
        mode |= 2;
    uintptr_t blit;
    switch (mode) {
    case 0x00008:
    case 0x40008:
        blit = PORT_ADDR(&RowBlitBlendAll);
        break;
    case 0x00002:
    case 0x20002:
        blit = PORT_ADDR(&RowBlitLookupAll);
        break;
    case 0x40002:
    case 0x60002:
        blit = PORT_ADDR(&RowBlitKeyedLookupAll);
        break;
    case 0x20008:
    case 0x60008:
        blit = PORT_ADDR(&RowBlend);
        break;
    case 0x0000a:
        blit = PORT_ADDR(&RowBlitLookupBlendAll);
        break;
    case 0x4000a:
        blit = PORT_ADDR(&RowBlitKeyedBlend);
        break;
    default:
        blit = PORT_ADDR(&RowBlitCopyAll);
        break;
    }
    if (g_ccVal220 != 0 && blit != static_cast<uint32_t>(PORT_ADDR(&RowBlend)) && Obj_OverlapsCircle(this) != 0) {
        Art_DrawClipped(blit, art, clip, sx, sy);
        if (outline != 0)
            Art_DrawClipped(blit, art, clip, sx + 5, sy + 5);
    } else if (needClip != 0) {
        Art_WalkClipped(blit, art, clip, sx, sy);
        if (outline != 0)
            Art_WalkClipped(blit, art, clip, sx + 5, sy + 5);
    } else {
        Art_DrawUnclipped(blit, art, sx, sy);
    }
    if (restoreHue != 0)
        RAW_AT(uint16_t, this, 0x3e) = static_cast<uint16_t>(restoreHue - 0x1f4);
}
HOOK_VMETHOD(0x0042c730, CObj, Draw);
