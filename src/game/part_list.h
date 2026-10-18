// PartList: the list of parts of a multi object (house, boat). Built by
// BuildPartList from the multi records (files 0xd/0xe) and stored at
// CDynamicObj::partList (+0x94). Layout from decomp/pass2/0x0044ea40.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "obj.h"
#include "pos3.h"


struct PartEntry {          // 0x14 bytes, scalar deleting destructor 0x0044eca0
    // 0x0044eca0: scalar deleting destructor, RET 4. Runs 0x0040caf0, frees when bit 0 of flags is set.
    void *__thiscall ScalarDeletingDtor(int flags);
    CObj *obj;              // +0x00
    int16_t x;              // +0x04
    int16_t y;              // +0x06
    int16_t z;              // +0x08
    uint8_t pad_0a[2];
    PartEntry *prev;        // +0x0c
    PartEntry *next;        // +0x10
};
static_assert(sizeof(PartEntry) == 0x14, "PartEntry size");
static_assert(offsetof(PartEntry, x) == 0x04, "PartEntry::x");
static_assert(offsetof(PartEntry, z) == 0x08, "PartEntry::z");
static_assert(offsetof(PartEntry, prev) == 0x0c, "PartEntry::prev");
static_assert(offsetof(PartEntry, next) == 0x10, "PartEntry::next");

struct PartList {           // 0x18 bytes (operator new in BuildPartList)
    PartEntry *head;        // +0x00
    PartEntry *tail;        // +0x04
    int32_t id;             // +0x08  multi id, -1 when empty
    int32_t locked;         // +0x0c
    int32_t maxExtent;      // +0x10  largest distance of a part from the origin (MapDistance)
    int32_t flag;           // +0x14

    // 0x0044e9a0  size 0x9c  move every part by (dx, dy, dz) and relink it into the world.
    // The two stack words are {dx | dy << 16} and dz, as the callers push them.
    void __thiscall Offset(Pos3 delta);
    // 0x0044e930: when not locked, delete every entry (vtable slot 4 of its object, then free) and reset the list.
    void __thiscall Clear();
    // 0x0044e990: RET 4. Set the locked flag (+0x0c).
    void __thiscall SetLocked(int value);
};
static_assert(sizeof(PartList) == 0x18, "PartList size");
static_assert(offsetof(PartList, tail) == 0x04, "PartList::tail");
static_assert(offsetof(PartList, id) == 0x08, "PartList::id");
static_assert(offsetof(PartList, locked) == 0x0c, "PartList::locked");
static_assert(offsetof(PartList, maxExtent) == 0x10, "PartList::maxExtent");
static_assert(offsetof(PartList, flag) == 0x14, "PartList::flag");

// 0x0044ea40  size 0x259  build the part list of multi `id` for `owner`; 0 when the multi is missing, else 1.
int __stdcall BuildPartList(int id, CDynamicObj *owner);
