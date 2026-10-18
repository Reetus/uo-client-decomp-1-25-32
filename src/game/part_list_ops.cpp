// PartList / PartEntry housekeeping (second pass, batch 19): clear, lock flag, entry destructor,
// (re)building the part list of a multi object, and the 12-byte multi record byte swap.
// The SEH frames of the original are omitted.
#include "pos3.h"
#include "part_list.h"

#include "../runtime/ported.h"
#include "obj.h"
#include "crt.h"
#include "packet_cursor.h"
#include "vcall.h"
#include "spell_icon_gump.h"

// 0x0040caf0: body of the entry destructor, plain RET (ECX = entry).
#define PartEntry_DtorBody(self) (Nop_0040caf0(self))

// 0x0044e930  size 0x5c
// Unless locked: delete every entry (vtable slot 4 of its object when still unlocked, destructor body,
// free), then clear head, tail and flag and set the id to -1.
void __thiscall PartList::Clear()
{
    PartEntry *e;
    PartEntry *nextEntry;

    if (locked == 0) {
        e = head;
        if (e != nullptr) {
            do {
                nextEntry = e->next;
                if (locked == 0)
                    decomp::vcall_void(e->obj, 0x04);
                if (e != nullptr) {
                    PartEntry_DtorBody(e);
                    operator delete(e);
                }
                e = nextEntry;
            } while (nextEntry != nullptr);
        }
        tail = nullptr;
        head = nullptr;
        flag = 0;
        id = -1;
    }
}
HOOK_METHOD(0x0044e930, PartList, Clear);

// 0x0044e990  size 0xd
void __thiscall PartList::SetLocked(int value)
{
    locked = value;
}
HOOK_METHOD(0x0044e990, PartList, SetLocked);

// 0x0044eca0  size 0x21
void *__thiscall PartEntry::ScalarDeletingDtor(int flags)
{
    PartEntry_DtorBody(this);
    if ((flags & 1) != 0)
        operator delete(this);
    return this;
}
HOOK_METHOD(0x0044eca0, PartEntry, ScalarDeletingDtor);

// 0x0044ecd0  size 0x162
// Make `obj` carry the part list of multi `id` at tile position pos = {x, y, z}: a list of another multi
// is deleted, a missing one is built (0x0044ea40). Without a list the object is just placed; with one it is
// moved unless it already sits at pos. ECX and EDX are not read (ECX is passed on to the builder, which
// ignores it); RET 0xc.
void __fastcall SetObjectPartList(void *unusedEcx, void *unusedEdx, CDynamicObj *obj, int16_t *pos, int id)
{
    (void)unusedEcx;
    (void)unusedEdx;
    PartList *list;
    PartEntry *e;
    PartEntry *nextEntry;

    list = obj->partList;
    if (list != nullptr && list->id != id) {
        if (list != nullptr) {
            if (list->locked == 0) {
                e = list->head;
                if (e != nullptr) {
                    do {
                        nextEntry = e->next;
                        if (list->locked == 0)
                            decomp::vcall_void(e->obj, 0x04);
                        if (e != nullptr) {
                            PartEntry_DtorBody(e);
                            operator delete(e);
                        }
                        e = nextEntry;
                    } while (nextEntry != nullptr);
                }
                list->id = -1;
                list->tail = nullptr;
                list->head = nullptr;
                list->flag = 0;
            }
            operator delete(list);
        }
        obj->partList = nullptr;
    }
    if (obj->partList == nullptr)
        BuildPartList(id, obj);
    if (obj->partList == nullptr) {
        if (obj->unlinked == 0)
            decomp::vcall_void(obj, 0x50);
        obj->ToWorld(*reinterpret_cast<uint32_t *>(pos), static_cast<uint16_t>(pos[2]));
        return;
    }
    if (obj->partList->flag == 1 && obj->inWorld == 1) {
        if (obj->unlinked != 0)
            goto place;
        if (static_cast<uint16_t>(pos[0]) == static_cast<uint16_t>(obj->x) && static_cast<uint16_t>(pos[1]) == static_cast<uint16_t>(obj->y) &&
            static_cast<uint16_t>(pos[2]) == static_cast<uint16_t>(obj->z))
            return;
    }
    if (obj->unlinked == 0)
        decomp::vcall_void(obj, 0x50);
place:
    obj->ToWorld(*reinterpret_cast<uint32_t *>(pos), static_cast<uint16_t>(pos[2]));
    list = obj->partList;
    if (list != nullptr)
        list->Offset(*reinterpret_cast<Pos3 *>(pos));
}
HOOK_FUNC(0x0044ecd0, SetObjectPartList);

// 0x0044ee40  size 0xde
// Copy `size` bytes of 12-byte records {u16 a, b, c, d; u32 e} into a new block, converting every field
// from network to host order (ntohs 0x0049862e, ntohl 0x00498616). RET 8.
void *__stdcall ByteSwapRecords12(const uint8_t *src, int size)
{
    uint8_t *dst = static_cast<uint8_t *>(::operator new(static_cast<size_t>(size)));
    int count = size / 12;
    const uint8_t *s = src;
    uint8_t *d = dst;

    while (count > 0) {
        uint16_t r0, r1, r2, r3;
        uint32_t r4;
        uint16_t w0, w1, w2, w3;
        uint32_t w4;
        memcpy(&r0, s + 0, 2);
        memcpy(&r1, s + 2, 2);
        memcpy(&r2, s + 4, 2);
        memcpy(&r3, s + 6, 2);
        memcpy(&r4, s + 8, 4);
        w0 = ntohs(r0);
        w1 = ntohs(r1);
        w2 = ntohs(r2);
        w3 = ntohs(r3);
        w4 = ntohl(r4);
        memcpy(d + 0, &w0, 2);
        memcpy(d + 2, &w1, 2);
        memcpy(d + 4, &w2, 2);
        memcpy(d + 6, &w3, 2);
        memcpy(d + 8, &w4, 4);
        d += 12;
        s += 12;
        count--;
    }
    return dst;
}
HOOK_FUNC(0x0044ee40, ByteSwapRecords12);
