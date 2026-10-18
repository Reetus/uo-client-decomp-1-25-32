// Placeholder classes for the two cleanup procedures at 0x00444090 / 0x004440e0 (the owner is the object at
// 0x005285e0 that WinMain releases at shutdown). The container keeps a singly linked list of 0x34-byte entries
// (next at +0x30); each entry owns a few heap blocks and an array of owned items.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

// 0x34-byte entry of the list.
class CEntry_004440e0 {
public:
    // 0x004440e0: destroy and free the owned items, free the five owned blocks and clear the entry.
    void __thiscall Release();

    uint8_t inUse;          // +0x00  cleared by Release
    void *block4;           // +0x04  owned block
    void *block8;           // +0x08  owned block
    int32_t unk_0c;
    int32_t unk_10;
    int32_t unk_14;
    int32_t unk_18;
    int32_t unk_1c;
    int32_t itemCount;      // +0x20  number of items in `items`
    void **items;           // +0x24  array of owned items (each is destroyed through 0x0047d430, then freed)
    void *block28;          // +0x28  owned block
    void *block2c;          // +0x2c  owned block
    CEntry_004440e0 *next;  // +0x30  next entry
};
static_assert(offsetof(CEntry_004440e0, itemCount) == 0x20, "CEntry_004440e0::itemCount");
static_assert(offsetof(CEntry_004440e0, items) == 0x24, "CEntry_004440e0::items");
static_assert(offsetof(CEntry_004440e0, next) == 0x30, "CEntry_004440e0::next");
static_assert(sizeof(CEntry_004440e0) == 0x34, "CEntry_004440e0 size");

class CEntryList_00444090 {
public:
    // 0x00444090: when initialised, release and free every entry and the block at +0x0c, then clear the flag.
    void __thiscall Release();

    uint8_t inited;         // +0x00
    uint8_t pad_01[0xb];
    void *block;            // +0x0c  owned block
    uint8_t pad_10[4];
    CEntry_004440e0 *first; // +0x14  first entry
};
static_assert(offsetof(CEntryList_00444090, block) == 0x0c, "CEntryList_00444090::block");
static_assert(offsetof(CEntryList_00444090, first) == 0x14, "CEntryList_00444090::first");
