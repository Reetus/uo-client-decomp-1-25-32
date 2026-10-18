// CWorldObj: base class of everything that is placed in the world (vtable 0x0049e908,
// base ctor 0x0042ad20, base dtor 0x0042ad70). Objects live in 64x64 buckets (head table
// at 0x0069fff8, indexed (y & 0x3f) * 64 + (x & 0x3f)) as a doubly linked list sorted by
// depth. Layout from decomp/pass2/0x0042ad70.cpp and 0x0042b4f0.cpp; derived classes
// (CObj, CTerrainTile, CMissileHelper) start their own fields at +0x34 or later.
//
// The class has real virtual functions (the 21 slots of the root table 0x0049e908, slot order = declaration order);
// the vptr at +0 is the compiler's and the head below starts at +4. See docs/matching.md "World-object virtual slots".
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

class CWorldObj;

// The first 0x20 bytes of a world object, up to the position.
struct CWorldObjHead {
    // The compiler's vptr is at +0x00; the first virtual lives here so that the whole chain is single inheritance with a
    // polymorphic root (a vptr introduced by CWorldObj would sit in front of the Head and make every pointer to member
    // of the hierarchy a two-word pointer). The slot 0 destructor is overridden all the way down.
    virtual ~CWorldObjHead() {}
    int32_t rectX0;         // +0x04 (after the compiler's vptr at +0x00)  screen rect (world scroll not subtracted)
    int32_t rectY0;         // +0x08
    int32_t rectX1;         // +0x0c
    int32_t rectY1;         // +0x10
    CWorldObj *next;        // +0x14  next in bucket (greater depth)
    CWorldObj *prev;        // +0x18
    int32_t inWorld;        // +0x1c
};

// The position x, y, z (+0x20, 6 bytes). The original destroys it through the empty destructor 0x0040caf0 in the
// unwind funclets of 0x0042ad70, so it is a class of its own with an empty destructor, between the head and the
// rest of CWorldObj (a base keeps the field names and stays single inheritance).
class CWorldObjPos : public CWorldObjHead {
public:
    int16_t x;              // +0x20
    int16_t y;              // +0x22
    int16_t z;              // +0x24
    ~CWorldObjPos() {}
};

class CWorldObj : public CWorldObjPos {
public:
    // 0x0042ad20: constructor, vtable 0x0049e908, prev / next / inWorld cleared.
    CWorldObj();
    // 0x0042ad70: destructor (slot 0 is the scalar deleting destructor 0x0042ad40). While in the world: invalidate the
    // screen rect and unlink from the world bucket list.
    virtual ~CWorldObj();
    // The old name of the destructor, for callers that take its address.
    void __thiscall Dtor() { this->~CWorldObj(); }

    // ---- the virtual slots, in slot order (offset = 4 * slot) ----
    // slot 1 (0x00454e50): delete this object through its virtual slot 0 with flag 1 (nothing for a null this).
    virtual void __thiscall DeleteSelf();
    // slots 2, 3: _purecall in the root table.
    virtual void __thiscall Draw(int *clip) = 0;
    virtual void __thiscall ComputeScreenRect() = 0;
    // slot 4 (0x0042b2d0): depth = z.
    virtual void __thiscall AddToWorld();
    // slot 5 (0x0042ae40): invalidate the screen rect (when shown) and unlink from the bucket list.
    virtual void __thiscall RemoveFromWorld();
    // slots 6 .. 14 and 16: constants (U1_Return0 / U1_Return1 in the original; the linker folds the copies).
    virtual int __thiscall Slot18() { return 0; }
    virtual int __thiscall Slot1c() { return 0; }
    virtual int __thiscall Slot20() { return 0; }
    virtual int __thiscall Slot24() { return 0; }
    virtual int __thiscall Slot28() { return 0; }
    virtual int __thiscall Slot2c() { return 1; }
    virtual int __thiscall Slot30() { return 0; }
    virtual int __thiscall Slot34() { return 0; }
    virtual int __thiscall Slot38() { return 0; }
    // slot 15 (0x0042c680 in the root).
    virtual int __thiscall IsHidden();
    // slot 16.
    virtual uint32_t __thiscall TileFlags() { return 0; }
    // slot 17 (0x00406640): the 16-bit value at +0x24, no stack argument (upper EAX not touched).
    virtual uint32_t __thiscall GetTopZ();
    // slot 18: the same body as slot 17 (identical code, folded by /OPT:ICF into 0x00406640; no HOOK of its own).
    virtual uint32_t __thiscall GetBaseZ();
    // slot 19 (0x00406650): the 16-bit value at +0x24; the stack argument is ignored (RET 4).
    virtual uint16_t __thiscall GetHeightAt(int unused);
    // slot 20 (0x00406720): does nothing.
    virtual void __thiscall ToVoid();

    // ---- non-virtual ----
    // 0x0042b1d0: RET 8. Skews (*px, *py) by +0xb8, inserts into that bucket sorted by depth, marks in world.
    void __thiscall InsertIntoBucket(int *px, int *py);
    // 0x0042bcf0: invalidate the screen rect when in world.
    void __thiscall Invalidate();
    // 0x0042bd40: mobile: invalidate the shown equipment and, for the player, the see-through circle.
    void __thiscall InvalidateEquipment();

    int32_t key28;         // +0x28  scrollBase + x - y
    int32_t sumXY;          // +0x2c  x + y
    int32_t depth;          // +0x30  z, adjusted by vtable slots 0x40 / 0x6c
};

static_assert(sizeof(CWorldObjHead) == 0x20, "vptr + the head");
static_assert(offsetof(CWorldObj, rectX0) == 0x04, "CWorldObj::rectX0");
static_assert(offsetof(CWorldObj, rectY1) == 0x10, "CWorldObj::rectY1");
static_assert(offsetof(CWorldObj, next) == 0x14, "CWorldObj::next");
static_assert(offsetof(CWorldObj, prev) == 0x18, "CWorldObj::prev");
static_assert(offsetof(CWorldObj, inWorld) == 0x1c, "CWorldObj::inWorld");
static_assert(offsetof(CWorldObj, x) == 0x20, "CWorldObj::x");
static_assert(offsetof(CWorldObj, y) == 0x22, "CWorldObj::y");
static_assert(offsetof(CWorldObj, z) == 0x24, "CWorldObj::z");
static_assert(offsetof(CWorldObj, key28) == 0x28, "CWorldObj::key28");
static_assert(offsetof(CWorldObj, sumXY) == 0x2c, "CWorldObj::sumXY");
static_assert(offsetof(CWorldObj, depth) == 0x30, "CWorldObj::depth");
static_assert(sizeof(CWorldObj) == 0x34, "CWorldObj fields end at +0x34");

// Intermediate class of the tile pool classes (0x0049f730 and the land tile 0x0049f788): both put the object in the world
// with the same slot-4 routine, 0x0042b3e0 (depth = z - 2). No data of its own.
class CWorldObjLow : public CWorldObj {
public:
    // slot 4 (0x0042b3e0).
    virtual void __thiscall AddToWorld();
};
static_assert(sizeof(CWorldObjLow) == 0x34, "CWorldObjLow adds no data");

// 0x00406720 as a plain callback / destructor stub (the root's empty ToVoid, slot 0x50): a pointer to the virtual member would
// be its call thunk. The code is the same single `ret`; the linker folds the copies.
void __cdecl Nop_00406720();
