// Third porting pass, batch u3: the vtable-only methods of the world object classes (CObj, CDynamicObj, CMobile and
// the 0x0049f9e0 class), the delete-self stub shared by eight vtables, and three small list / buffer helpers that
// have no direct caller. Companion of obj.cpp.
#include <cstdint>

#include "obj.h"

#include "../runtime/ported.h"
#include "crt.h"
#include "part_list.h"
#include "raw_field.h"
#include "vcall.h"
#include "art_record_ops_ports.h"
#include "object_manager.h"
#include "obj_extra_ports.h"
#include "vtables.h"
#include "globals.h"

#define g_tileDataTable GLOBAL_AS(const uint8_t *, g_tileData)   // 0x28-byte tiledata records
#define ART_RECORDS_OBJ GLOBAL_PTR(void, g_artCache_storage)

// Tiledata record of this object's graphic: index = graphic + (signed byte at +0x3a), 0x28 bytes each.
static const uint8_t *TileRecordOf(const CObj *o)
{
    int index = static_cast<int>(static_cast<int8_t>(o->unk_3a)) + static_cast<int>(o->graphic);
    return g_tileDataTable + index * 0x28;
}

// 0x00451c50  size 0x7
// Vtable slot 0x64 of the CObj classes (8 vtables): bit 0 of flags74.
int __thiscall CObj::IsOnList70()
{
    return flags74 & 1;
}
HOOK_VMETHOD(0x00451c50, CObj, IsOnList70);

// 0x00451c60  size 0x5
// Vtable slot 0x68 of the CObj classes (8 vtables): set bit 0 of flags74.
void __thiscall CObj::SetOnList70Flag()
{
    flags74 = flags74 | 1;
}
HOOK_VMETHOD(0x00451c60, CObj, SetOnList70Flag);

// 0x00451e70  size 0x100
// Vtable slot 0x04 of the 0x0049f860 class (CDynamicObj): give the item back to the dynamic pool. Decrements the live
// count, frees the +0xa0 buffer and the part list, detaches the two gumps that name this object as owner, clears the
// selected object, leaves the world (virtual slot 0x50) when linked, unlinks from the item list (head 0x0068ce7c, next
// +0x80, prev +0x84), pushes onto the free list at 0x00721060 (next in +0x80) and deletes every bark gump at +0x68.
void __thiscall CDynamicObj::DeleteSelf()
{
    g_pool_dynLive = g_pool_dynLive - 1;
    if (buf_a0 != nullptr)
        operator delete(buf_a0);
    PartList *parts = partList;
    if (parts != nullptr) {
        PORT_FUNC1(&PartList::Clear, void, __fastcall, PartList *)(parts);   // PartList::Clear
        operator delete(parts);
    }
    if (gump88 != nullptr) {
        if (gump88->owner == this)
            gump88->owner = nullptr;
    }
    if (gump8c != nullptr) {
        if (gump8c->owner == this)
            gump8c->owner = nullptr;
    }
    if (this == g_selectedObj)
        g_selectedObj = nullptr;
    if (unlinked == 0)
        decomp::vcall_void(this, 0x50);
    if (listNext != nullptr)
        listNext->listPrev = listPrev;
    else
        g_dynListTail = listPrev;
    if (listPrev != nullptr)
        listPrev->listNext = listNext;
    else
        g_dynListHead = listNext;
    listNext = reinterpret_cast<CDynamicObj *>(g_pool_dynFree);
    g_pool_dynFree = reinterpret_cast<char *>(this);
    g_childDeleteFlag = 1;
    while (barkList != nullptr)
        decomp::vdelete(barkList);
    g_childDeleteFlag = 0;
}
HOOK_VMETHOD(0x00451e70, CDynamicObj, DeleteSelf);

// 0x00453260  size 0x1e
// Vtable slot 0x6c: height, the byte at +0x12 of the graphic's tiledata record.
int __thiscall CObj::Height()
{
    return TileRecordOf(this)[0x12];
}
HOOK_VMETHOD(0x00453260, CObj, Height);

// 0x00453280  size 0x18
// Vtable slot 0x40: the tiledata flags (first dword of the graphic's record).
uint32_t __thiscall CObj::TileFlags()
{
    return *reinterpret_cast<const uint32_t *>(TileRecordOf(this));
}
HOOK_VMETHOD(0x00453280, CObj, TileFlags);

// 0x004532a0  size 0x1f
// Vtable slot 0x70: the word at +0x10 of the graphic's tiledata record.
int __thiscall CObj::TileWord10()
{
    return *reinterpret_cast<const uint16_t *>(TileRecordOf(this) + 0x10);
}
HOOK_VMETHOD(0x004532a0, CObj, TileWord10);

// 0x00453300  size 0x1d
// Vtable slot 0x7c: 1 when the graphic is 0x28e or 0xeed.
int __thiscall CObj::IsGraphic28eOrEed()
{
    uint32_t g = graphic;
    if (g == 0x28e || g == 0xeed)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x00453300, CObj, IsGraphic28eOrEed);

// 0x00453df0  size 0x6
// Vtable slot 0x6c of the 0x0049f9e0 class: height 6.
int __thiscall CObjVt49f9e0::Height()
{
    return 6;
}
HOOK_VMETHOD(0x00453df0, CObjVt49f9e0, Height);

// 0x00454050  size 0x6
// Vtable slot 0x6c of CMobile and CPlayer: height 0xf.
int __thiscall CMobile::Height()
{
    return 0xf;
}
HOOK_VMETHOD(0x00454050, CMobile, Height);

// 0x004545a0  size 0x18
// Vtable slot 0x84 of CMobile and CPlayer: the word at +0x1a8 when +0x1a4 is nonzero, else -1.
int __thiscall CMobile::AnimWordOrMinus1()
{
    if (unk_1a4 != 0)
        return unk_1a8;
    return -1;
}
HOOK_VMETHOD(0x004545a0, CMobile, AnimWordOrMinus1);

// 0x00454700  size 0x6
// Vtable slot 0x70 of CMobile and CPlayer: 1.
int __thiscall CMobile::TileWord10()
{
    return 1;
}
HOOK_VMETHOD(0x00454700, CMobile, TileWord10);

// 0x00454710  size 0x1d0
// Vtable slot 0x74 of CMobile and CPlayer: the art record (0x00476130 on the art object at 0x0068d2d8) for the body
// graphic, picked by a switch (5, 6, 0xc8 and the 13 graphics 0xc9 .. 0x124 of the jump table at 0x0045484c, indexed
// through the byte table at 0x00454884); null for any other graphic.
void *__thiscall CMobile::ArtRecord()
{
    uint32_t g = graphic;
    if (static_cast<int>(g) > 0xc8) {
        switch (g) {
        case 0xc9: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x611b);
        case 0xcc: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6124);
        case 0xcd: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6125);
        case 0xd3: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6118);
        case 0xd4: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x611e);
        case 0xd6: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6119);
        case 0xd9: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x611c);
        case 0xe1: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6122);
        case 0xe2: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6120);
        case 0xe4: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6121);
        case 0xee: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6123);
        case 0x123: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6126);
        case 0x124: return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x6127);
        default: return nullptr;
        }
    }
    if (g == 0xc8)
        return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x611f);
    if (g == 5)
        return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x611d);
    if (g == 6)
        return PORT_THISCALL1(&ArtRecord_Get, void *, ART_RECORDS_OBJ, int)(0x611a);
    return nullptr;
}
HOOK_VMETHOD(0x00454710, CMobile, ArtRecord);

// 0x00454ce0  size 0x19
// Vtable slot 0x8c of CMobile and CPlayer: 1 when the graphic is 0x192 or 0x193.
int __thiscall CMobile::IsGhostBody()
{
    if (graphic == 0x192 || graphic == 0x193)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x00454ce0, CMobile, IsGhostBody);

// 0x00454e50  size 0xb
// Slot 0x04 of eight vtables (0x0049e908, 0x0049ebd0, the CObj classes): delete this object through its virtual slot 0
// with flag 1; nothing for a null this. ECX is the object, no stack arguments. EAX is the deleting destructor's result
// (this); with a null this the original leaves EAX as it found it, here 0.
void __thiscall CWorldObj::DeleteSelf()
{
    CWorldObj *const self = this;
    if (self == nullptr)
        return;
    decomp::vcall(PORT_RET(void *), self, 0x00, 1);
}
HOOK_VMETHOD(0x00454e50, CWorldObj, DeleteSelf);

// Slot 0x04 of the CContainerObj table: the same body as the root's (the linker folds the copies into 0x00454e50).
void __thiscall CContainerObj::DeleteSelf()
{
    CContainerObj *const self = this;
    if (self == nullptr)
        return;
    decomp::vcall(PORT_RET(void *), self, 0x00, 1);
}

// Slot 0x04 of the CObjVt49f960 table: the same body as the root's (the linker folds the copies into 0x00454e50).
void __thiscall CObjVt49f960::DeleteSelf()
{
    CObjVt49f960 *const self = this;
    if (self == nullptr)
        return;
    decomp::vcall(PORT_RET(void *), self, 0x00, 1);
}

// Slot 0x04 of the CObjVt49fb90 table: the same body as the root's (the linker folds the copies into 0x00454e50).
void __thiscall CObjVt49fb90::DeleteSelf()
{
    CObjVt49fb90 *const self = this;
    if (self == nullptr)
        return;
    decomp::vcall(PORT_RET(void *), self, 0x00, 1);
}

// Slot 0x04 of the CPlayer table: the same body as the root's (the linker folds the copies into 0x00454e50).
void __thiscall CPlayer::DeleteSelf()
{
    CPlayer *const self = this;
    if (self == nullptr)
        return;
    decomp::vcall(PORT_RET(void *), self, 0x00, 1);
}

// 0x004573b0  size 0x6a
// Empty the list {head +0, tail +4, count +8} of {next, prev, record} nodes (the pending label list of CObjectManager at
// +0x74): unlink the head node, delete it, run the record's text destructor (LabelRecord::DtorText 0x00457420) and delete
// the record, until the list is empty. ECX = list.
struct LabelListHead {
    uint32_t *head;
    uint32_t *tail;
    int32_t count;
};
static void __fastcall LabelList_Clear_004573b0(LabelListHead *list)
{
    uint32_t *node = list->head;
    if (node == nullptr)
        return;
    do {
        void *record = nullptr;
        uint32_t *nextNode;
        if (node == nullptr) {
            nextNode = nullptr;   // dead path of the original: the loop never enters with a null node
        } else {
            uint32_t *prevNode = reinterpret_cast<uint32_t *>(node[1]);
            nextNode = reinterpret_cast<uint32_t *>(node[0]);
            if (prevNode != nullptr)
                prevNode[0] = reinterpret_cast<uint32_t>(nextNode);
            else
                list->head = nextNode;
            if (nextNode != nullptr)
                nextNode[1] = reinterpret_cast<uint32_t>(prevNode);
            else
                list->tail = prevNode;
            record = reinterpret_cast<void *>(node[2]);
            node[2] = 0;
            operator delete(node);
            list->count = list->count - 1;
        }
        if (record != nullptr) {
            reinterpret_cast<LabelRecord *>(record)->DtorText();   // LabelRecord::DtorText
            operator delete(record);
        }
        node = nextNode;
    } while (node != nullptr);
}
HOOK_FUNC(0x004573b0, LabelList_Clear_004573b0);

// 0x004777d0  size 0x11
// Free the pointer at +8 of the object in ECX when it is set (the pointer is not cleared). No known caller.
static void __fastcall Obj_FreePtr8_004777d0(void *self)
{
    void *p = RAWF(void *, self, 8);
    if (p != nullptr)
        operator delete(p);
}
HOOK_FUNC(0x004777d0, Obj_FreePtr8_004777d0);

// 0x00480b10  size 0x4f
// Free a table of owned records: {items +0 (array of pointers), count +4}. For every non-null item the original walks
// the chain of +8 links to its end (the result is unused), deletes the block at item+0, then the item; finally it
// deletes the array. ECX = table.
struct PtrTable {
    uint32_t **items;
    int32_t count;
};
static void __fastcall PtrTable_FreeAll_00480b10(PtrTable *table)
{
    int i = 0;
    if (table->count > 0) {
        do {
            uint32_t *item = table->items[i];
            if (item != nullptr) {
                uint32_t *chain = item;
                do {
                    chain = reinterpret_cast<uint32_t *>(chain[2]);   // walk to the end of the +8 chain, unused
                } while (chain != nullptr);
            }
            if (item != nullptr) {
                operator delete(reinterpret_cast<void *>(item[0]));
                operator delete(item);
            }
            i++;
        } while (i < table->count);
    }
    operator delete(table->items);
}
HOOK_FUNC(0x00480b10, PtrTable_FreeAll_00480b10);
