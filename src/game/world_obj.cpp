// CWorldObj. The SEH frame of the original (try level 0 at entry) is omitted.
#include "world_obj.h"
#include "obj.h"

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "dirty_rects_ports.h"
#include "vtables.h"
#include "globals.h"

#define g_worldBuckets GLOBAL_PTR(CWorldObj *, g_tileBuckets)    // 64 x 64 bucket heads
// 0x0044f8b0: invalidate a screen rect (ECX = &{x0, y0, x1, y1}).
#define Screen_Invalidate(o) PORT_THISCALL0(&DirtyRects_Invalidate, void, (o))()

struct Rect4 {
    int x0, y0, x1, y1;
};

// 0x0042ad20  size 0x14
CWorldObj::CWorldObj()
{
    prev = nullptr;
    next = nullptr;
    inWorld = 0;
}
HOOK_CTOR(0x0042ad20, CWorldObj, ());

// 0x0042ad70  size 0xc9
CWorldObj::~CWorldObj()
{
    if (inWorld != 0) {
        int r = IsHidden();   // the root's slot 0x3c, called directly (0x0049e944)
        if (r == 0 && inWorld != 0) {
            Rect4 rect;
            rect.x0 = rectX0 - g_scrollX;
            rect.y0 = rectY0 - g_scrollY;
            rect.x1 = rectX1 - g_scrollX;
            rect.y1 = rectY1 - g_scrollY;
            Screen_Invalidate(&rect);
        }

        CWorldObj *nextObj = next;
        if (nextObj != nullptr)
            nextObj->prev = prev;
        CWorldObj *prevObj = prev;
        if (prevObj != nullptr) {
            prevObj->next = next;
        } else {
            unsigned int by = static_cast<uint8_t>(y);
            unsigned int bx = static_cast<uint8_t>(x);
            g_worldBuckets[((by & 0x3f) << 6) + (bx & 0x3f)] = next;     // bucket head = this->next
        }
        inWorld = 0;
        prev = nullptr;
        next = nullptr;
    }
}
HOOK_DTOR(0x0042ad70, CWorldObj);
HOOK_VTABLE(0x0049e908, CWorldObj);
HOOK_SDD(0x0042ad40, CWorldObj);
// The trivial slots: the original has one copy of `return 0` and one of `return 1` (0x0040c7c0 / 0x0040c7d0, ports U1_Return0 / U1_Return1);
// the linker folds the inline virtuals of the hierarchy into one function each, which pairs with those two as well.
HOOK_VMETHOD(0x0040c7c0, CWorldObj, Slot18);
HOOK_VMETHOD(0x0040c7d0, CWorldObj, Slot2c);
HOOK_DTOR(0x0040caf0, CWorldObjPos);

// 0x0042b4f0  size 0x126
void __thiscall CObj::AddToWorld()
{
    depth = z;
    sumXY = x + y;
    key28 = g_scrollBase + x - y;

    unsigned int r40 = PORT_VSLOT0(this, 0x40, unsigned int)();
    if (r40 & 1)
        depth = depth - 1;
    int r6c = PORT_VSLOT0(this, 0x6c, int)();
    if (r6c > 0)
        depth = depth + 1;

    int by = static_cast<int>(y);
    int bx = static_cast<int>(x);
    int r30 = PORT_VSLOT0(this, 0x30, int)();
    if (r30 != 0) {
        int skew = RAW_AT(int32_t, this, 0xb8);
        bx = (bx + skew) & 0x3f;
        by = (by - skew) & 0x3f;
    }
    by = by & 0x3f;
    bx = bx & 0x3f;
    int idx = (by << 6) + bx;

    CWorldObj **bucket = g_worldBuckets + idx;
    CWorldObj *head = *bucket;
    CWorldObj *cur = head;
    CWorldObj *prevObj = nullptr;
    if (cur != nullptr) {
        do {
            if (cur->depth > depth)
                break;
            prevObj = cur;
            cur = cur->next;
        } while (cur != nullptr);
    }

    if (prevObj != nullptr) {
        prev = prevObj;
        next = prevObj->next;
        prevObj->next = this;
        CWorldObj *n = next;
        if (n != nullptr)
            n->prev = this;
    } else {
        next = head;
        if (head != nullptr)
            head->prev = this;
        *bucket = this;
        prev = nullptr;
    }

    inWorld = 1;
    PORT_VSLOT0(this, 0x0c, void)();
    int r3c = PORT_VSLOT0(this, 0x3c, int)();
    if (r3c == 0 && inWorld != 0) {
        Rect4 rect;
        rect.x0 = rectX0 - g_scrollX;
        rect.y0 = rectY0 - g_scrollY;
        rect.x1 = rectX1 - g_scrollX;
        rect.y1 = rectY1 - g_scrollY;
        Screen_Invalidate(&rect);
    }
}
HOOK_VMETHOD(0x0042b4f0, CObj, AddToWorld);
