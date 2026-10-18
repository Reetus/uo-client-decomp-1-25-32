#include "multi_placer.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "object_manager.h"
#include "crt.h"
#include "raw_field.h"
#include "class_0049c690.h"
#include "world_obj_ops.h"
#include "file_manager.h"
#include "spell_icon_gump.h"
#include "small_methods_u1_ports.h"

#define g_multiFiles GLOBAL_PTR(void, g_fileManager_storage)                // data file pair reader (files 0xd / 0xe)

// 0x0040cf20  size 0x3 (a lone ret) and 0x0040caf0  size 0x1 (a lone ret): element constructor and destructor of the table entries.
MultiEntry::MultiEntry() {}
HOOK_CTOR(0x0040cf20, MultiEntry, ());
MultiEntry::~MultiEntry() {}
HOOK_DTOR(0x0040caf0, MultiEntry);

// 0x0040cb00  size 0x39
CMultiPlacer::CMultiPlacer()
{
    g_multiPlacer = this;
    unk_000 = 0;
    tileX = -1;
    unk_110 = -1;
    unk_100 = 0;
    zOffset = 0;
    loaded = 0;
    reinterpret_cast<CWorldObj *>(this)->CWorldObj::ToVoid();   // a lone `ret`
}
HOOK_CTOR(0x0040cb00, CMultiPlacer, ());

// 0x0040cb40  size 0x127
// Place: create one 0x0049fb90 object per component of the loaded multi at (tileX + dx, tileY + dy),
// each with curZ = ground height + zOffset + dz, and invalidate the sprite of each. The SEH frame is
// omitted. The original does not check the constructor result before reading obj+0x18.
void __thiscall CMultiPlacer::Place(int x, int y)
{
    int ground = (&g_objMgr)->CObjectManager::GroundZ(x, y);
    int base = ground + zOffset;
    int i = 0;
    int off = 0;
    while (i < GLOBAL_AS(MultiTable *, g_loadedMulti)->count) {
        MultiEntry *e = reinterpret_cast<MultiEntry *>(reinterpret_cast<char *>(GLOBAL_AS(MultiTable *, g_loadedMulti)->entries) + off);
        int cx = x + e->dx;
        int cy = y + e->dy;
        curZ = e->dz + base;
        CObjVt49fb90 *obj = new CObjVt49fb90(GLOBAL_AS(MultiTable *, g_loadedMulti)->entries[off / 0x10].graphic,
                                             static_cast<uint16_t>(cx), static_cast<uint16_t>(cy));
        void *sprite = RAWF(void *, obj, 0x18);
        if (sprite != nullptr)
            reinterpret_cast<CWorldObj *>(sprite)->Invalidate();   // CWorldObj::Invalidate
        i++;
        off += 0x10;
    }
    baseZ = base;
    tileX = x;
    tileY = y;
}
HOOK_METHOD(0x0040cb40, CMultiPlacer, Place);

// 0x0040cc70  size 0x59
void __thiscall CMultiPlacer::FreeTable()
{
    MultiTable *table = GLOBAL_AS(MultiTable *, g_loadedMulti);
    if (table != nullptr) {
        MultiEntry *arr = table->entries;
        if (arr != nullptr) {
            delete[] arr;
        }
        operator delete(table);
        GLOBAL_AS(MultiTable *, g_loadedMulti) = nullptr;
    }
    zOffset = 0;
    curZ = 0;
}
HOOK_METHOD(0x0040cc70, CMultiPlacer, FreeTable);

// 0x0040ccd0  size 0x244
// LoadMulti: read the multi's 12-byte records (id, x, y, z, flags) and build the table of 16-byte
// entries with (offX, offY, offZ) subtracted from the position. The previous table is released
// without clearing the global, as in the original. The SEH frame is omitted.
int __thiscall CMultiPlacer::LoadMulti(int multiId, int16_t offX, int16_t offY, int16_t offZ)
{
    int dataSize = 0;
    int extra = 0;
    int16_t *src = reinterpret_cast<int16_t *>(
        reinterpret_cast<CFileManager *>(g_multiFiles)->GetRecord(0xd, 0xe, multiId, &dataSize, &extra));
    if (src == nullptr)
        return 0;
    int count = dataSize / 12;
    MultiTable *old = GLOBAL_AS(MultiTable *, g_loadedMulti);
    if (old != nullptr) {
        MultiEntry *arr = old->entries;
        if (arr != nullptr) {
            delete[] arr;
        }
        operator delete(old);
    }
    MultiTable *table = new MultiTable;
    GLOBAL_AS(MultiTable *, g_loadedMulti) = table;

    MultiEntry *entries = new MultiEntry[count];
    GLOBAL_AS(MultiTable *, g_loadedMulti)->entries = entries;
    GLOBAL_AS(MultiTable *, g_loadedMulti)->count = count;
    strcpy(GLOBAL_AS(MultiTable *, g_loadedMulti)->name, g_multiStrIndex);
    if (count > 0) {
        const int16_t *rec = src;   // record: id, x, y, z, flags (u32)
        int off = 0;
        for (int left = count; left != 0; left--) {
            int16_t id = rec[0];
            int16_t rx = rec[1];
            int16_t ry = rec[2];
            int16_t rz = rec[3];
            MultiEntry *e = reinterpret_cast<MultiEntry *>(reinterpret_cast<char *>(GLOBAL_AS(MultiTable *, g_loadedMulti)->entries) + off);
            e->dx = rx;
            e->dy = ry;
            e->dz = rz;
            e->dx = static_cast<int16_t>(e->dx - offX);
            e->dy = static_cast<int16_t>(e->dy - offY);
            e->dz = static_cast<int16_t>(e->dz - offZ);
            e->graphic = static_cast<uint16_t>(id);
            e->pad_08 = 0;
            *reinterpret_cast<int32_t *>(e->pad_0c) = 0;
            rec += 6;
            off += 0x10;
        }
    }
    tileX = -1;
    loaded = 1;
    return 1;
}
HOOK_METHOD(0x0040ccd0, CMultiPlacer, LoadMulti);

// 0x0040cf30  size 0x30
// Delete every object of the 0x0049fb90 list (head at 0x0068ce88 = CObjectManager::firstFb90), invalidating
// the sprite of each first; the destructor unlinks the object from the list.
void __cdecl MultiObjects_DeleteAll()
{
    CObj *obj = g_objMgr.firstFb90;
    if (obj == nullptr)
        return;
    do {
        void *sprite = RAWF(void *, obj, 0x18);
        if (sprite != nullptr) {
            reinterpret_cast<CWorldObj *>(sprite)->Invalidate();   // CWorldObj::Invalidate
            obj = g_objMgr.firstFb90;
        }
        if (obj == nullptr)
            return;
        PORT_VSLOT1(obj, 0x00, void *, int)(1);   // deleting destructor
        obj = g_objMgr.firstFb90;
    } while (obj != nullptr);
}
HOOK_FUNC(0x0040cf30, MultiObjects_DeleteAll);
