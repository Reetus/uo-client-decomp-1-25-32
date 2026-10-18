// The class with vtable 0x0049c690 (name not established). Objects keep a link in a
// sibling list owned by `owner`: prev/next at +0x4c/+0x50, the owner's list head
// (its most recent child) at owner+0xb0. Layout is partial; see decomp/pass2/0x00406690.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "world_obj.h"

class CObj0049c690 : public CWorldObj {
public:
    // Batch u1 (small_methods_u1.cpp).
    // 0x0040cf60: vtable slot 0x04 (also a button callback / confirm dialog "no"): delete the object in ECX.
    virtual void __thiscall DeleteSelf();
    // 0x004067a0: vtable slot 0x3c: tail call of the same slot of the owner (+0x48).
    virtual int __thiscall IsHidden();
    // 0x00406690: destructor (its exception frame has state -1 only around the base destructor call).
    virtual ~CObj0049c690();
    // 0x004065b0: RET 0x14. Links the new object at the head of owner's list (owner+0xb0) and copies
    // owner+0x04 / +0x0c / +0x28 / +0x30 into itself.
    CObj0049c690(int16_t w20, int16_t w22, int p40, int p44, CObj0049c690 *ownerObj);
    // 0x00406730: virtual slot 0x08. Clips `rect` {x0, y0, x1, y1} to y in [+0x40, +0x44] minus the scroll
    // offset and draws through the owner's slot 0x08.
    virtual void __thiscall Draw(int *rect);
    // slot 0x0c: the empty body 0x00406720.
    virtual void __thiscall ComputeScreenRect();
    virtual int __thiscall Slot38() { return 1; }

    uint8_t pad_34[0xc];    // +0x34
    int32_t top40;          // +0x40  clip range of the slice
    int32_t bottom44;       // +0x44
    CObj0049c690 *owner;    // +0x48  holds the list head at owner+0xb0
    CObj0049c690 *prev;     // +0x4c
    CObj0049c690 *next;     // +0x50
};

// 0x0040cf60 as a button callback (ECX = gump): delete the object through its scalar deleting destructor. The same code is the
// slot 1 of this class; a pointer to a virtual member would be the call thunk, so the callbacks take this function.
void __fastcall DeleteGumpCallback(void *self);

static_assert(offsetof(CObj0049c690, owner) == 0x48, "CObj0049c690::owner");
static_assert(offsetof(CObj0049c690, prev) == 0x4c, "CObj0049c690::prev");
static_assert(offsetof(CObj0049c690, next) == 0x50, "CObj0049c690::next");
static_assert(sizeof(CObj0049c690) == 0x54, "CObj0049c690 size (operator new 0x54)");
