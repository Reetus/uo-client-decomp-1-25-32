#include "pos3.h"
#include "part_list.h"

#include "../runtime/ported.h"
#include "map_world.h"
#include "crt.h"
#include "vcall.h"
#include "file_manager.h"
#include "obj.h"
#include "globals.h"

// 0x004405d0: fetch a record from a data file pair (thiscall on the file manager
// at 0x0068d980). Returns the record data and fills in its length.
static uint8_t *LoadRecord(int fileA, int fileB, int index, int *len, int *unused)
{
    return reinterpret_cast<CFileManager *>(GLOBAL_PTR(void, g_fileManager_storage))->GetRecord(fileA, fileB, index, len, unused);
}

// 0x0044e9a0  size 0x9c
// Move every part of the list by (dx, dy, dz) and relink it into the world. The SEH frame is omitted.
void __thiscall PartList::Offset(Pos3 delta)
{
    Pos3 np;
    PartEntry *e = head;
    if (e != nullptr) {
        do {
            np.x = static_cast<int16_t>(e->x + delta.x);
            np.y = static_cast<int16_t>(e->y + delta.y);
            np.z = static_cast<int16_t>(e->z + delta.z);
            if (e->obj->unlinked == 0)
                decomp::vcall_void(e->obj, 0x50);   // obj->vtbl[20]()
            // 8 bytes pushed: {x, y} dword then z word; ECX = e->obj
            PORT_THISCALL1(&CObj::ToWorld, void, e->obj, Pos3)(np);   // 0x00452580
            e = e->next;
        } while (e != nullptr);
    }
    flag = 1;
}
HOOK_METHOD(0x0044e9a0, PartList, Offset);

// 0x0044ea40  size 0x259
// Build the part list of multi `id` for `owner` (multi records from files 0xd/0xe, 12 bytes each:
// graphic.w, x.w, y.w, z.w, kind.dw). Part 0 only sets the owner's graphic; parts of kind 1 become
// new 0x78-byte objects. The SEH frame is omitted.
int __stdcall BuildPartList(int id, CDynamicObj *owner)
{
    int len = 0;
    int unused = 0;
    int maxExtent = 0;
    uint8_t *data = LoadRecord(0xd, 0xe, id, &len, &unused);
    if (data == nullptr)
        return 0;

    int count = len / 12;
    PartList *list = owner->partList;
    if (list != nullptr) {
        if (list->locked == 0) {
            PartEntry *e = list->head;
            while (e != nullptr) {
                PartEntry *next = e->next;
                if (list->locked == 0)
                    decomp::vcall_void(e->obj, 4);   // entry->obj->vtbl[1]()
                if (e != nullptr)
                    decomp::thiscall_at_void(PORT_ADDR(&PartEntry::ScalarDeletingDtor), e, 1);   // scalar deleting destructor
                e = next;
            }
            list->tail = nullptr;
            list->head = nullptr;
            list->flag = 0;
            list->id = -1;
        }
        operator delete(list);
        owner->partList = nullptr;
    }

    list = static_cast<PartList *>(::operator new(sizeof(PartList)));
    if (list != nullptr) {
        list->tail = nullptr;
        list->head = nullptr;
        list->id = -1;
        list->locked = 0;
        list->maxExtent = 0;
        list->flag = 1;
    }
    owner->partList = list;
    list->id = id;   // no null check, as in the binary

    for (int i = 0; i < count; i++) {
        uint16_t graphic = *reinterpret_cast<uint16_t *>(data + i * 12);
        int16_t pos[3];
        pos[0] = *reinterpret_cast<int16_t *>(data + i * 12 + 2);
        pos[1] = *reinterpret_cast<int16_t *>(data + i * 12 + 4);
        pos[2] = *reinterpret_cast<int16_t *>(data + i * 12 + 6);
        uint32_t kind = *reinterpret_cast<uint32_t *>(data + i * 12 + 8);
        if (i == 0) {
            owner->graphic = graphic;
        } else if (kind == 1) {
            CObj *obj = new CObj();
            obj->graphic = graphic;
            PartList *pl = owner->partList;
            PartEntry *e = static_cast<PartEntry *>(::operator new(sizeof(PartEntry)));
            decomp::vcall_void(obj, 0x68);   // obj->vtbl[26]()
            e->obj = obj;
            e->x = pos[0];
            e->y = pos[1];
            e->z = pos[2];
            PartEntry *tail = pl->tail;
            e->prev = tail;
            if (tail != nullptr)
                tail->next = e;
            PartEntry *oldHead = pl->head;
            pl->tail = e;
            if (oldHead == nullptr)
                pl->head = e;
            e->next = nullptr;
            obj->CObj::LinkList70();
            int ext = MapDistance(pos, 0, 0);
            if (ext > maxExtent)
                maxExtent = ext;
        }
    }
    owner->partList->maxExtent = maxExtent;
    owner->partList->flag = 0;
    return 1;
}
HOOK_FUNC(0x0044ea40, BuildPartList);
