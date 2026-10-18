// CCircleTrans: the "see through" circle drawn around the player's position when a roof or wall would hide him.
// One object at 0x004b71f8 (also reached as g_spriteBlitter from sprite_draw.cpp): sprite runs inside the circle are
// blended with what is already on the back buffer. Layout from the assembly of 0x00424300 .. 0x00424570.
// Also here: the list of 0x004248b0 .. 0x00424940 (a doubly linked list of nodes pointing at objects) and the
// world object test 0x004244e0 that decides whether an object is drawn through the circle.
// (Porting pass 2, batch 08.)
#pragma once

#include <cstddef>
#include <cstdint>

#include "world_obj.h"

class CCircleTrans {
public:
    // 0x00424300: RET 4. Radius clamped to [2, 0x100], then Recompute.
    void __thiscall SetRadius(int r);
    // 0x00424330: RET 8. Centre clamped to the 640x480 screen, then Recompute.
    void __thiscall SetCenter(int cx, int cy);
    // 0x00424370: plain RET. radius^2, its shift when it is a power of two, bounding rect and the blend function.
    void __thiscall Recompute();
    // 0x004243e0: RET 0xc. Blend of two 5-5-5 pixels with weights (r2 - w, w), radius^2 a power of two.
    uint32_t __thiscall BlendPow2(uint32_t a, uint32_t b, int w);
    // 0x00424460: RET 0xc. Same with a division by radius^2.
    uint32_t __thiscall BlendGeneric(uint32_t a, uint32_t b, int w);
    // 0x00424570: RET 0x10. Draw `count` pixels from `src` to `dest` with the copy routine `blit`
    // (__fastcall(dest, src, count), RET 4), blending those inside the circle.
    void __thiscall BlendSpan(uint32_t blit, uint16_t *dest, uint16_t *src, int count);

    int32_t rect[4];       // +0x00  bounding rect (left, top, right, bottom) of the circle
    int32_t radius;        // +0x10
    int32_t radiusSq;      // +0x14
    int32_t shift;         // +0x18  log2(radiusSq) when a power of two, else 0
    int32_t centerX;       // +0x1c
    int32_t centerY;       // +0x20
    uint32_t blendFn;      // +0x24  code address of BlendPow2 or BlendGeneric
};

static_assert(offsetof(CCircleTrans, radius) == 0x10, "CCircleTrans::radius");
static_assert(offsetof(CCircleTrans, radiusSq) == 0x14, "CCircleTrans::radiusSq");
static_assert(offsetof(CCircleTrans, shift) == 0x18, "CCircleTrans::shift");
static_assert(offsetof(CCircleTrans, centerX) == 0x1c, "CCircleTrans::centerX");
static_assert(offsetof(CCircleTrans, centerY) == 0x20, "CCircleTrans::centerY");
static_assert(offsetof(CCircleTrans, blendFn) == 0x24, "CCircleTrans::blendFn");

// A world object (screen rect at +0x04, z at +0x24) as seen by 0x004244e0.
class CWorldObj_004244e0 : public CWorldObj {
public:
    // 0x004244e0: plain RET. Non-zero when vtable slot 0x2c says 0, the object reaches more than 5 above the
    // player and its screen rect (scrolled) overlaps the circle's bounding rect.
    int __thiscall OverlapsCircle();
};

struct CObjListNode {
    CObjListNode *next;    // +0x00
    CObjListNode *prev;    // +0x04
    void *obj;             // +0x08  object whose +0x1bc is cleared when the node leaves the list
};

// List head: first node at +0, last node at +4.
class CObjList {
public:
    // 0x004248b0: plain RET.
    void __thiscall Init();
    // 0x004248c0: RET 4.
    void __thiscall Remove(CObjListNode *node);
    // 0x00424920: RET 4. Node whose obj is `obj`, or 0.
    CObjListNode *__thiscall Find(void *obj);
    // 0x00424940: plain RET. Unlinks and frees every node.
    void __thiscall Clear();

    CObjListNode *head;    // +0x00
    CObjListNode *tail;    // +0x04
};

static_assert(offsetof(CObjListNode, obj) == 0x08, "CObjListNode::obj");
static_assert(offsetof(CObjList, tail) == 0x04, "CObjList::tail");
