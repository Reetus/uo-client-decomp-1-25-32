// CCircleTrans, the world object test 0x004244e0 and the object list 0x004248b0 .. 0x00424940.
// (Porting pass 2, batch 08.)
#include "circle_trans.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "world_renderer.h"
#include "world_obj_ops.h"
#include "art_row_blits_hue_ports.h"

#define g_circleTrans GLOBAL_AS(CCircleTrans, g_circleTrans_storage)

// 0x0042bea0: ECX = rect a, EDX = rect b; non-zero when they overlap. Plain RET.
#define Orig_RectsOverlap(a1, a2) (WorldRectsOverlap(a1, a2))
// 0x0042bcf0: ECX = object. Invalidates its screen rect. Plain RET.
#define Orig_ObjInvalidate(self) (reinterpret_cast<CWorldObj *>(self)->Invalidate())

// Copy routines handed to BlendSpan: ECX = dest, EDX = src, stack = pixel count (RET 4).
typedef void(__fastcall *PixelBlit)(uint16_t *, const uint16_t *, int);

// 0x00424300  size 0x28
void __thiscall CCircleTrans::SetRadius(int r)
{
    int v = r;
    if (v < 2)
        v = 2;
    if (v > 0x100)
        v = 0x100;
    radius = v;
    Recompute();   // 0x00424370
}
HOOK_METHOD(0x00424300, CCircleTrans, SetRadius);

// 0x00424330  size 0x3d
void __thiscall CCircleTrans::SetCenter(int cx, int cy)
{
    int px = cx;
    int py = cy;
    if (px < 0)
        px = 0;
    if (px >= 0x280)
        px = 0x27f;
    if (py < 0)
        py = 0;
    if (py >= 0x1e0)
        py = 0x1df;
    centerX = px;
    centerY = py;
    Recompute();   // 0x00424370
}
HOOK_METHOD(0x00424330, CCircleTrans, SetCenter);

// 0x00424370  size 0x6a
// radius^2, the shift (floor(log2) of it) when it has a single bit set, the blend routine for that case and the
// bounding rect (centre -/+ (radius + 1)).
void __thiscall CCircleTrans::Recompute()
{
    int r = radius;
    int v = r * r;
    int bits;

    radiusSq = v;
    shift = 0;
    bits = 0;
    while (v != 0) {
        if (v & 1)
            bits++;
        v >>= 1;
        if (v != 0)
            shift++;
    }
    if (bits != 1) {
        shift = 0;
        blendFn = PORT_ADDR(&CCircleTrans::BlendGeneric);   // BlendGeneric
    } else {
        blendFn = PORT_ADDR(&CCircleTrans::BlendPow2);   // BlendPow2
    }
    rect[0] = centerX - radius - 1;
    rect[2] = centerX + radius + 1;
    rect[1] = centerY - radius - 1;
    rect[3] = centerY + radius + 1;
}
HOOK_METHOD(0x00424370, CCircleTrans, Recompute);

// 0x004243e0  size 0x74
uint32_t __thiscall CCircleTrans::BlendPow2(uint32_t a, uint32_t b, int w)
{
    int sa = static_cast<int>(a);
    int sb = static_cast<int>(b);
    int inv = radiusSq - w;
    int sh = shift;
    int hi = ((sb >> 10) * inv + (sa >> 10) * w) >> sh;
    int mid = ((((sa >> 5) & 0x1f) * w) + (((sb >> 5) & 0x1f) * inv)) >> sh;
    int lo = ((sb & 0x1f) * inv + (sa & 0x1f) * w) >> sh;
    return static_cast<uint32_t>((((hi << 5) | mid) << 5) | lo);
}
HOOK_METHOD(0x004243e0, CCircleTrans, BlendPow2);

// 0x00424460  size 0x7c
uint32_t __thiscall CCircleTrans::BlendGeneric(uint32_t a, uint32_t b, int w)
{
    int sa = static_cast<int>(a);
    int sb = static_cast<int>(b);
    int rsq = radiusSq;
    int inv = rsq - w;
    int hi = ((sa >> 10) * w + (sb >> 10) * inv) / rsq;
    int mid = ((((sa >> 5) & 0x1f) * w) + (((sb >> 5) & 0x1f) * inv)) / rsq;
    int lo = ((sa & 0x1f) * w + (sb & 0x1f) * inv) / rsq;
    return static_cast<uint32_t>((((hi << 5) | mid) << 5) | lo);
}
HOOK_METHOD(0x00424460, CCircleTrans, BlendGeneric);

// 0x004244e0  size 0x82
int __thiscall CWorldObj_004244e0::OverlapsCircle()
{
    int r[4];

    if (decomp::vcall(PORT_RET(int), this, 0x2c) == 0) {
        int h = decomp::vcall(PORT_RET(int), this, 0x6c);
        if (h + static_cast<int16_t>(z) > static_cast<int16_t>(g_player->z) + 5) {
            int sx = g_scrollX;
            r[0] = rectX0 - sx;
            int sy = g_scrollY;
            r[1] = rectY0 - sy;
            r[2] = rectX1 - sx;
            r[3] = rectY1 - sy;
            if (Orig_RectsOverlap(r, g_circleTrans.rect) != 0)
                return 1;
        }
    }
    return 0;
}
HOOK_METHOD(0x004244e0, CWorldObj_004244e0, OverlapsCircle);

// 0x00424570  size 0x16e
// Pixel index of `dest` in the back buffer gives the screen position; pixels whose squared distance from the centre
// is within radius^2 are blended (source over destination, weight = distance^2), the rest are plain copies by
// `blit`. A row whose dy^2 already exceeds radius^2 is one plain blit of the whole span.
void __thiscall CCircleTrans::BlendSpan(uint32_t blit, uint16_t *dest, uint16_t *src, int count)
{
    uint32_t pixel = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dest) - reinterpret_cast<uintptr_t>(g_backBuffer)) >> 1;
    int px = static_cast<int>(pixel) % 0x280 - centerX;
    int py = static_cast<int>(pixel) / 0x280;
    int dy = py - centerY;
    int dy2 = dy * dy;
    int dx = px;
    PixelBlit copy = reinterpret_cast<PixelBlit>(static_cast<uintptr_t>(blit));
    typedef uint32_t (decomp::Any::*BlendFn)(uint32_t, uint32_t, int);
    BlendFn blend = decomp::pmf_cast<BlendFn>(decomp::code_ptr(blendFn)).p;

    if (blit == static_cast<uint32_t>(PORT_ADDR(&RowBlitCopyAll))) {
        if (dy2 > radiusSq) {
            copy(dest, src, count);
            return;
        }
        if (count <= 0)
            return;
        for (;;) {
            int d2 = dx * dx + dy2;
            if (d2 > radiusSq) {
                copy(dest, src, 1);
            } else {
                uint32_t a = *src;
                uint32_t b = *dest;
                *dest = static_cast<uint16_t>((((decomp::Any *)this)->*blend)(a, b, d2));
            }
            dx++;
            dest++;
            src++;
            count--;
            if (count == 0)
                break;
        }
        return;
    }
    if (dy2 > radiusSq) {
        copy(dest, src, count);
        return;
    }
    if (count <= 0)
        return;
    for (;;) {
        int d2 = dx * dx + dy2;
        if (d2 > radiusSq) {
            copy(dest, src, 1);
        } else {
            uint32_t temp = (static_cast<uint32_t>(dy2) & 0xffff0000u) | *dest;
            copy(reinterpret_cast<uint16_t *>(&temp), src, 1);
            uint32_t a = temp & 0xffff;
            uint32_t b = *dest;
            *dest = static_cast<uint16_t>((((decomp::Any *)this)->*blend)(a, b, d2));
        }
        dx++;
        dest++;
        src++;
        count--;
        if (count == 0)
            break;
    }
}
HOOK_METHOD(0x00424570, CCircleTrans, BlendSpan);

// 0x004242b0  size 0x7
// Empty procedure with two stack arguments (ECX ignored).
void __stdcall CircleTrans_Nullsub(int a, int b)
{
    (void)a;
    (void)b;
}
HOOK_FUNC(0x004242b0, CircleTrans_Nullsub);

// 0x004248b0  size 0x10
void __thiscall CObjList::Init()
{
    tail = nullptr;
    head = nullptr;
}
HOOK_METHOD(0x004248b0, CObjList, Init);

// 0x004248c0  size 0x5c
// Clears obj+0x1bc, invalidates the object, unlinks the node.
void __thiscall CObjList::Remove(CObjListNode *node)
{
    RAWF(int32_t, node->obj, 0x1bc) = 0;
    Orig_ObjInvalidate(node->obj);   // 0x0042bcf0
    if (tail == node)
        tail = node->prev;
    if (head == node)
        head = node->next;
    if (node->prev != nullptr)
        node->prev->next = node->next;
    if (node->next != nullptr)
        node->next->prev = node->prev;
    node->prev = nullptr;
    node->next = nullptr;
}
HOOK_METHOD(0x004248c0, CObjList, Remove);

// 0x00424920  size 0x1d
CObjListNode *__thiscall CObjList::Find(void *obj)
{
    for (CObjListNode *n = head; n != nullptr; n = n->next) {
        if (n->obj == obj)
            return n;
    }
    return nullptr;
}
HOOK_METHOD(0x00424920, CObjList, Find);

// 0x00424940  size 0x6c
// Same unlinking as Remove for every node, then operator delete on the node.
void __thiscall CObjList::Clear()
{
    CObjListNode *n = head;
    while (n != nullptr) {
        CObjListNode *following = n->next;
        RAWF(int32_t, n->obj, 0x1bc) = 0;
        Orig_ObjInvalidate(n->obj);   // 0x0042bcf0
        if (tail == n)
            tail = n->prev;
        if (head == n)
            head = n->next;
        if (n->prev != nullptr)
            n->prev->next = n->next;
        if (n->next != nullptr)
            n->next->prev = n->prev;
        n->prev = nullptr;
        n->next = nullptr;
        operator delete(n);
        n = following;
    }
}
HOOK_METHOD(0x00424940, CObjList, Clear);
