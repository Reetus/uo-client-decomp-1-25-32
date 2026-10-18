// CObjectManager: ground height, deleting every object, the tiledata patch / load procedures and the world
// packet object creation / update (AddOrUpdateObject). 0x00454d00 .. 0x00455760 (porting pass 2, batch 20).
// The SEH frames of the original are omitted.
#include "object_manager.h"

#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "map_world.h"
#include "light_tables_ports.h"
#include "circle_trans_ports.h"
#include "cell_grid.h"
#include "part_list.h"
#include "part_list_ops_ports.h"
#include "obj_pool_alloc_ports.h"
#include "path_search.h"
#include "map_gump_ops_ports.h"
#include "text_gump_ctor_ports.h"
#include "vtables.h"

// Globals (Ghidra addresses).
#define g_pickerFlag GLOBAL_AS(int, g_worldPicker_storage)           // first word of the picker object; zero when the map is used
#define g_animObjList GLOBAL_AS(CObj *, g_objectManager_storage)       // objects with tiledata flag 0x1000000 (next at +0x50)
#define g_pathSearch GLOBAL_PTR(void, g_pathSearch_storage)          // the A* search object
#define g_msgTarget GLOBAL_PTR(void, g_msgTarget_storage)

struct PathNode;

// Original code not ported here.
#define Map_NeighborZ(self, a1, a2) ((signed char *)reinterpret_cast<CMap *>(self)->GetNeighborZ(a1, a2))    // ECX = map object (CMap::GetNeighborZ)
#define Item_Invalidate(o) PORT_THISCALL0(&InvalidateLightRect, void, (o))()                      // ECX = item
#define Nullsub_4242b0(a1, a2) (CircleTrans_Nullsub(a1, a2))      // ECX = 0x00528698 is ignored
#define CellGrid_RelinkById PORT_FUNC1(&CellGrid_RelinkById, void, __stdcall, int)      // (ECX = 0x0069e0c0 is ignored)
#define CellGrid_InvalidateTerrainTile PORT_FUNC1(&CellGrid_InvalidateTerrainTile, void, __stdcall, uint32_t)
#define PartList_SetBusy(self, a1) (reinterpret_cast<PartList *>(self)->SetLocked(a1))                // ECX = part list, flag
#define GetObjectAt(self, a1, a2, a3) ((int)reinterpret_cast<CWorldPicker *>(self)->GetObjectAt(a1, a2, a3))            // ECX = 0x0069e0c0 picker
#define SetObjectPartList PORT_FUNC5(&SetObjectPartList, void, __fastcall, void *, void *, CDynamicObj *, int16_t *, int)
#define Item_PoolNew(a1, a2, a3) ((CObj *)DynamicObj_PoolNew(a1, a2, (uint32_t)(a3)))   // serial, graphic, offset byte
#define PathSearch_FreeLists(self) (reinterpret_cast<CPathSearch *>(self)->FreeLists())
#define PathSearch_Run(self, a1, a2, a3, a4, a5, a6, a7, a8) (reinterpret_cast<CPathSearch *>(self)->Run(a1, a2, a3, a4, a5, a6, a7, a8))
#define Path_ReverseLinks() (PathReverseLinks())
#define Journal_Say(o, a1, a2, a3) PORT_THISCALL3(&CreateBarkGump, void, (o), int, int, const char *)(a1, a2, a3)  // ECX = 0x006486b0 (ptr, 3, text)

// One step of a mobile's walk queue as written by AddOrUpdateObject (10 bytes at 0x154 + n * 10).
static int16_t &StepX(CMobile *m, int n) { return RAWF(int16_t, m, 0x154 + n * 10); }
static int16_t &StepY(CMobile *m, int n) { return RAWF(int16_t, m, 0x156 + n * 10); }
static int16_t &StepZ(CMobile *m, int n) { return RAWF(int16_t, m, 0x158 + n * 10); }
static uint16_t &StepGraphic(CMobile *m, int n) { return RAWF(uint16_t, m, 0x15a + n * 10); }
static uint8_t &StepFacing(CMobile *m, int n) { return RAWF(uint8_t, m, 0x15c + n * 10); }
static uint8_t &StepStatus(CMobile *m, int n) { return RAWF(uint8_t, m, 0x15d + n * 10); }

// ---------------------------------------------------------------------------
// Scalar deleting destructor of the 0x0049fb90 class
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Ground height and teardown
// ---------------------------------------------------------------------------

// 0x00454d00  size 0x98
// While DAT_0069e0c0 is zero, start from the mean of two corner heights of the map cell (x, y)
// (bytes 1 and 3 of 0x00446030's result, halved toward zero); otherwise from 0. Every object of the world list
// at that tile that is not hidden (virtual slot 0x3c) raises it to tiledata height (+0x12) + its z.
// A result of -0x7f means "nothing": 0. RET 8.
int __thiscall CObjectManager::GroundZ(int x, int y)
{
    int best = 0;
    if (g_pickerFlag == 0) {
        signed char *cell = Map_NeighborZ(g_mapObject, x, y);   // 0x00446030
        int sum = cell[3] + cell[1];
        best = sum / 2;
    }
    for (CObj *o = listTail; o != nullptr; o = o->link40) {
        if (o->x == x && o->y == y && decomp::vcall(PORT_RET(int), o, 0x3c) == 0) {
            int tile = o->graphic + static_cast<int8_t>(o->unk_3a);
            int top = tileItems[tile * 0x28 + 0x12] + o->z;
            if (top > best)
                best = top;
        }
    }
    if (best == -0x7f)
        return 0;
    return best;
}
HOOK_METHOD(0x00454d00, CObjectManager, GroundZ);

// 0x00455090  size 0x8c
// Delete every object but the player: first the player is taken out of the world and its 25 equipment slots
// (+0xc4 .. +0x128) destroyed through virtual slot 4, then the world list is walked (next at +0x40, read before
// the delete); objects with virtual slot 0x2c set get their part list (+0x94) flagged first.
void __thiscall CObjectManager::DeleteAllObjects()
{
    CObj *player = g_player;
    if (player != nullptr) {
        if (player->unlinked == 0)
            decomp::vcall_void(player, 0x50);
        for (int off = 0xc4; off < 0x12c; off += 4) {
            CObj *item = RAWF(CObj *, g_player, off);
            if (item != nullptr)
                decomp::vcall_void(item, 4);
        }
    }
    CObj *o = listTail;
    while (o != nullptr) {
        CObj *next = o->link40;
        if (o != g_player) {
            if (decomp::vcall(PORT_RET(int), o, 0x2c) != 0) {
                void *parts = RAWF(void *, o, 0x94);
                if (parts != nullptr)
                    PartList_SetBusy(parts, 1);   // 0x0044e990
                decomp::vcall_void(o, 4);
            } else {
                decomp::vcall_void(o, 4);
            }
        }
        o = next;
    }
}
HOOK_METHOD(0x00455090, CObjectManager, DeleteAllObjects);

// 0x00455120  size 0x1b
// RET 8. The picker at 0x0069e0c0 is asked for the object under (x, y) with flag 1 (0x0042fd50).
int __stdcall ObjectManager_GetObjectAtScreen(int x, int y)
{
    return GetObjectAt(GLOBAL_PTR(void, g_worldPicker_storage), x, y, 1);   // 0x0042fd50
}
HOOK_FUNC(0x00455120, ObjectManager_GetObjectAtScreen);

// ---------------------------------------------------------------------------
// tiledata.mul
// ---------------------------------------------------------------------------

// 0x00455140  size 0x32c
// Patch the item tiledata group `group` (record index base group * 32 after the 0x200 land groups) from the
// 37-byte records at `data` (preceded by the 4-byte group header), in memory and in tiledata.mul (opened r+b,
// group at file offset 0x68800 + (group - 0x200) * 0x4a4). A record whose flags or +0xe word change relinks
// the objects of that graphic. When any record changed bit 0x800000 or 0x1000000 the animated and light lists
// are rebuilt: all light-list members are taken off it, the animated chain is cleared and every object of the
// world list is put back according to its tile flags. RET 8.
void __thiscall CObjectManager::SetItemTileData(int group, const uint8_t *data)
{
    int changed = 0;
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("tiledata.mul")), "r+b");   // 0x0044c340
    const uint8_t *p = data;
    uint32_t header = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    Nullsub_4242b0(group + g_tileGroupBase, static_cast<int>(header));   // 0x004242b0
    int g = group - 0x200;
    fseek(f, g * 0x4a4 + 0x68800, 0);
    fwrite(&header, 4, 1, f);
    int tileIndex = g * 32;
    int off = g * 0x500;
    for (int n = 0x20; n != 0; n--) {
        uint8_t *rec = tileItems + off;
        uint32_t newFlags = *reinterpret_cast<const uint32_t *>(p);
        uint32_t oldFlags = *reinterpret_cast<uint32_t *>(rec);
        if ((oldFlags ^ newFlags) & 0x1800000)
            changed = 1;
        if (oldFlags != newFlags || *reinterpret_cast<uint16_t *>(rec + 0xe) != *reinterpret_cast<const uint16_t *>(p + 0xc))
            CellGrid_RelinkById(tileIndex);   // 0x00430880
        *reinterpret_cast<uint32_t *>(tileItems + off) = *reinterpret_cast<const uint32_t *>(p);
        fwrite(p, 4, 1, f);
        tileItems[off + 4] = p[4];
        fwrite(p + 4, 1, 1, f);
        tileItems[off + 5] = p[5];
        fwrite(p + 5, 1, 1, f);
        *reinterpret_cast<uint32_t *>(tileItems + off + 8) = *reinterpret_cast<const uint32_t *>(p + 6);
        fwrite(p + 6, 4, 1, f);
        *reinterpret_cast<uint16_t *>(tileItems + off + 0xc) = *reinterpret_cast<const uint16_t *>(p + 10);
        fwrite(p + 10, 2, 1, f);
        *reinterpret_cast<uint16_t *>(tileItems + off + 0xe) = *reinterpret_cast<const uint16_t *>(p + 12);
        fwrite(p + 12, 2, 1, f);
        *reinterpret_cast<uint16_t *>(tileItems + off + 0x10) = *reinterpret_cast<const uint16_t *>(p + 14);
        fwrite(p + 14, 2, 1, f);
        tileItems[off + 0x12] = p[16];
        fwrite(p + 16, 1, 1, f);
        strncpy(reinterpret_cast<char *>(tileItems + off + 0x13), reinterpret_cast<const char *>(p + 17), 0x14);   // 0x0048b750
        fwrite(tileItems + off + 0x13, 1, 0x14, f);
        p += 37;
        off += 0x28;
        tileIndex++;
    }
    fclose(f);
    if (changed == 0)
        return;

    // rebuild the light list and the animated list
    CObj *o = reinterpret_cast<CObj *>(unk_08);
    while (o != nullptr) {
        if (o->onLightList != 0) {
            if (g_lightListHead == o)
                g_lightListHead = o->link48;
            if (g_lightListTail == o)
                g_lightListTail = o->link4c;
            if (o->link48 != nullptr)
                o->link48->link4c = o->link4c;
            if (o->link4c != nullptr)
                o->link4c->link48 = o->link48;
            o->onLightList = 0;
            Item_Invalidate(o);   // 0x0044f140
        }
        o = reinterpret_cast<CObj *>(unk_08);
    }
    CObj *a = reinterpret_cast<CObj *>(unk_20);
    while (a != nullptr) {
        CObj *following = a->link50;
        a->link54 = nullptr;
        a->link50 = nullptr;
        a = following;
    }
    CObj *w = listTail;
    unk_20 = 0;
    while (w != nullptr) {
        if (*reinterpret_cast<uint32_t *>(g_tileData + w->graphic * 0x28) & 0x1000000) {
            CObj *head = reinterpret_cast<CObj *>(unk_20);
            w->link54 = nullptr;
            w->link50 = head;
            if (head != nullptr)
                head->link54 = w;
            unk_20 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(w));
        }
        if (decomp::vcall(PORT_RET(uint32_t), w, 0x40) & 0x800000) {
            if (w->onLightList == 0) {
                w->onLightList = 1;
                CObj *oldHead = g_lightListHead;
                w->link48 = oldHead;
                if (oldHead != nullptr)
                    oldHead->link4c = w;
                w->link4c = nullptr;
                CObj *oldTail = g_lightListTail;
                g_lightListHead = w;
                if (oldTail == nullptr)
                    g_lightListTail = w;
                Item_Invalidate(w);   // 0x0044f140
            }
        }
        w = w->link40;
    }
}
HOOK_METHOD(0x00455140, CObjectManager, SetItemTileData);

// 0x00455470  size 0x14c
// Patch the terrain tiledata group `group` (records of 26 bytes in the file, 0x1c in memory) the same way: the
// group header, then per record the flags dword, the +4 word (terrain objects with another word are
// invalidated through 0x004307d0 first) and the 20-byte name. RET 8.
void __thiscall CObjectManager::SetTerrTileData(int group, const uint8_t *data)
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("tiledata.mul")), "r+b");   // 0x0044c340
    const uint8_t *p = data;
    uint32_t header = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    Nullsub_4242b0(group + g_tileGroupBase, static_cast<int>(header));   // 0x004242b0
    fseek(f, group * 836, 0);
    fwrite(&header, 4, 1, f);
    int tileIndex = group * 32;
    int off = group * 0x380;
    for (int n = 0x20; n != 0; n--) {
        if (*reinterpret_cast<uint16_t *>(tileTerrain + off + 4) != *reinterpret_cast<const uint16_t *>(p + 4))
            CellGrid_InvalidateTerrainTile(static_cast<uint32_t>(tileIndex));   // 0x004307d0
        *reinterpret_cast<uint32_t *>(tileTerrain + off) = *reinterpret_cast<const uint32_t *>(p);
        fwrite(p, 4, 1, f);
        *reinterpret_cast<uint16_t *>(tileTerrain + off + 4) = *reinterpret_cast<const uint16_t *>(p + 4);
        fwrite(p + 4, 2, 1, f);
        strncpy(reinterpret_cast<char *>(tileTerrain + off + 6), reinterpret_cast<const char *>(p + 6), 0x14);   // 0x0048b750
        fwrite(tileTerrain + off + 6, 1, 0x14, f);
        p += 26;
        off += 0x1c;
        tileIndex++;
    }
    fclose(f);
}
HOOK_METHOD(0x00455470, CObjectManager, SetTerrTileData);

// 0x004555c0  size 0x194
// Read tiledata.mul (opened with the string at 0x004a60e0): 0x70000 / 0x1c land records (a 4-byte group header
// skipped before every 32nd), then 0xa0000 / 0x28 item records, field by field into the two buffers.
void __thiscall CObjectManager::LoadTileData()
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("tiledata.mul")), g_fopenReadMode);   // 0x0044c340
    if (f == nullptr)
        return;
    int count = 0;
    for (int off = 0; off < 0x70000; off += 0x1c) {
        if ((count & 0x1f) == 0)
            fseek(f, 4, 1);
        fread(tileTerrain + off, 4, 1, f);
        fread(tileTerrain + off + 4, 2, 1, f);
        fread(tileTerrain + off + 6, 0x14, 1, f);
        count++;
    }
    count = 0;
    for (int off = 0; off < 0xa0000; off += 0x28) {
        if ((count & 0x1f) == 0)
            fseek(f, 4, 1);
        fread(tileItems + off, 4, 1, f);
        fread(tileItems + off + 4, 1, 1, f);
        fread(tileItems + off + 5, 1, 1, f);
        fread(tileItems + off + 8, 4, 1, f);
        fread(tileItems + off + 0xc, 2, 1, f);
        fread(tileItems + off + 0xe, 2, 1, f);
        fread(tileItems + off + 0x10, 2, 1, f);
        fread(tileItems + off + 0x12, 1, 1, f);
        fread(tileItems + off + 0x13, 0x14, 1, f);
        count++;
    }
    fclose(f);
}
HOOK_METHOD(0x004555c0, CObjectManager, LoadTileData);

// ---------------------------------------------------------------------------
// World packet object creation / update
// ---------------------------------------------------------------------------

// 0x00455760  size 0x7da
// Create or update the object `serial` from a world packet. Positions near the edge of the 0x1400 x 0x1000 world
// are wrapped around the view origin first; a graphic of 0x4000 or more is a part of a multi (part index graphic -
// 0x4000, graphic then 1). A serial with bit 30 set is an item: graphic 0x2006 is built inline as the 0x124-byte
// multi class, tiledata flag 0x100000 makes a 0xb4-byte 0x0049f960 object, 0x200000 a 0xbc-byte container, anything
// else comes from the 0x78 / 0xa8 pool (0x004516a0); the new item gets facing, quantity, sanitized hue and status
// flags. Without the bit it is a mobile (operator new 0x1dc, CMobile::Ctor3). An existing item that is not a
// mobile and has no part list is taken out of the world first. Items are then updated and placed (ToWorld, or the
// part list builder); mobiles go through the walk queue: a full queue (4 steps) snaps them to the packet position,
// otherwise the packet position is appended as a step unless it repeats the newest one. Finally a followed
// player target farther than 3 tiles starts a path search ("Can't get there (follow)" on failure), a new item flushes
// its labels, and an object that is out of range 0x12 is destroyed unless virtual slot 0x64 keeps it. RET 0x28.
void __thiscall CObjectManager::AddOrUpdateObject(uint32_t serial, uint32_t graphic, int8_t offset, uint16_t qty, int x,
                                                  int y, int z, uint8_t facing, uint32_t hue, uint32_t flags)
{
    int partIndex = -1;     // [ebp-0x10]
    int isNew = 0;          // [ebp-0x18]
    int16_t pos[3];         // [ebp-0x24]: x, y, z words

    if (g_viewCenterX < 0x1400) {
        if (g_viewCenterX < 0x12) {
            if (x > 0x13ee)
                x = x - 0x1400;
        }
        if (g_viewCenterX >= 0x13ee) {
            if (x <= 0x12)
                x = x + 0x1400;
        }
        if (g_viewCenterY < 0x12) {
            if (y > 0xfee)
                y = y - 0x1000;
        }
        if (g_viewCenterY >= 0xfee) {
            if (y <= 0x12)
                y = y + 0x1000;
        }
    }
    if (static_cast<uint16_t>(graphic) >= 0x4000) {
        partIndex = static_cast<int>(graphic & 0xffff) - 0x4000;
        graphic = 1;
    }

    CDynamicObj *obj = list24;
    while (obj != nullptr && static_cast<uint32_t>(obj->serial) != serial)
        obj = obj->listNext;

    if (obj == nullptr) {
        if (serial & 0x40000000) {
            if (static_cast<uint16_t>(graphic) == 0x2006) {
                obj = new CObjVt49f9e0(static_cast<int>(serial), 0x2006);   // 0x00453e30
            } else {
                uint32_t tileFlags = *reinterpret_cast<uint32_t *>(g_tileData + (graphic & 0xffff) * 0x28);
                if (tileFlags & 0x100000) {
                    obj = new CObjVt49f960(static_cast<int>(serial), static_cast<int>(graphic), offset, 1);   // 0x00453b20
                } else if (tileFlags & 0x200000) {
                    obj = new CContainerObj(static_cast<int>(serial), static_cast<int>(graphic), offset, 1);   // 0x00453930
                } else {
                    obj = static_cast<CDynamicObj *>(Item_PoolNew(serial, graphic & 0xffff, offset));   // 0x004516a0
                }
            }
            obj->facing = facing;
            obj->unk_3c = qty;
            obj->hue = static_cast<uint16_t>(CObj_SanitizeHue(hue & 0xffff));
            CDynamicObj_SetStatusFlags(obj, flags & 0xff);   // 0x004521a0
            isNew = 1;
        } else {
            CMobile *mob = new CMobile(static_cast<int>(serial), static_cast<int>(graphic), static_cast<uint8_t>(offset));   // 0x00454090
            mob->moving = (facing & 0xff) >> 7;
            mob->facing = facing & 0x7f;
            mob->hue = static_cast<uint16_t>(CObj_SanitizeHue(hue & 0xffff));
            mob->unk_188 = 0;
            mob->unk_18c = 0;
            pos[0] = static_cast<int16_t>(x);
            pos[1] = static_cast<int16_t>(y);
            pos[2] = static_cast<int16_t>(static_cast<int8_t>(z));
            mob->viewZ = static_cast<int8_t>(z);
            mob->ToWorld(static_cast<uint32_t>(static_cast<uint16_t>(pos[0])) | (static_cast<uint32_t>(static_cast<uint16_t>(pos[1])) << 16),
                         static_cast<uint16_t>(pos[2]));   // 0x00452580
            CDynamicObj_SetStatusFlags(mob, flags & 0xff);   // 0x004521a0
            mob->flag148 = 0;
            obj = mob;
        }
    } else {
        if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0 && obj->partList == nullptr)
            decomp::vcall_void(obj, 0x50);
    }

    if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0) {
        // an item (or a part of a multi)
        pos[1] = static_cast<int16_t>(y);
        pos[0] = static_cast<int16_t>(x);
        pos[2] = static_cast<int16_t>(static_cast<int8_t>(z));
        obj->facing = facing;
        obj->unk_3a = static_cast<uint8_t>(offset);
        obj->unk_3c = qty;
        CDynamicObj_SetStatusFlags(obj, flags & 0xff);   // 0x004521a0
        obj->hue = static_cast<uint16_t>(CObj_SanitizeHue(hue & 0xffff));
        if (partIndex == -1) {
            obj->graphic = static_cast<uint16_t>(graphic & 0x3fff);
            obj->ToWorld(static_cast<uint32_t>(static_cast<uint16_t>(pos[0])) | (static_cast<uint32_t>(static_cast<uint16_t>(pos[1])) << 16),
                         static_cast<uint16_t>(pos[2]));   // 0x00452580
        } else {
            SetObjectPartList(GLOBAL_PTR(void, g_006f4940_storage), nullptr, obj, pos, partIndex);   // 0x0044ecd0
        }
    } else {
        CMobile *mob = static_cast<CMobile *>(static_cast<CObj *>(obj));
        mob->unk_14c = static_cast<int32_t>(g_gameTick);
        if (mob->stepCount == 4) {
            int reenter = 0;
            if (mob->inWorld != 0) {
                decomp::vcall_void(mob, 0x14);
                reenter = 1;
            }
            mob->z = static_cast<int16_t>(static_cast<int8_t>(z));
            mob->graphic = static_cast<uint16_t>(graphic & 0x3fff);
            mob->facing = facing & 0x7f;
            mob->x = static_cast<int16_t>(x);
            mob->unk_13c = 1;
            mob->y = static_cast<int16_t>(y);
            mob->hue = static_cast<uint16_t>(CObj_SanitizeHue(hue & 0xffff));
            mob->stepCount = 0;
            CDynamicObj_SetStatusFlags(mob, flags & 0xff);   // 0x004521a0
            mob->flag148 = 0;
            if (reenter != 0)
                decomp::vcall_void(mob, 0x10);
        } else {
            if (mob->flag148 != 0) {
                if (mob->inWorld != 0) {
                    decomp::vcall_void(mob, 0x14);
                    mob->flag148 = 0;
                    decomp::vcall_void(mob, 0x10);
                } else {
                    mob->flag148 = 0;
                }
            }
            int n = mob->stepCount;
            int differs = 1;
            if (n == 0) {
                if (mob->x == x && mob->y == y && mob->z == static_cast<int16_t>(static_cast<int8_t>(z)) &&
                    ((mob->facing ^ facing) & 0x7f) == 0 &&
                    (mob->moving != 0 ? 1 : 0) == ((facing & 0xff) >> 7) && mob->graphic == static_cast<uint16_t>(graphic) &&
                    mob->flags98 == static_cast<uint8_t>(flags))
                    differs = 0;
            } else {
                int last = n - 1;
                if (StepX(mob, last) == x && StepY(mob, last) == y &&
                    StepZ(mob, last) == static_cast<int16_t>(static_cast<int8_t>(z)) && StepFacing(mob, last) == facing &&
                    StepGraphic(mob, last) == static_cast<uint16_t>(graphic) && StepStatus(mob, last) == static_cast<uint8_t>(flags))
                    differs = 0;
            }
            if (differs != 0) {
                StepX(mob, mob->stepCount) = static_cast<int16_t>(x);
                StepY(mob, mob->stepCount) = static_cast<int16_t>(y);
                StepZ(mob, mob->stepCount) = static_cast<int16_t>(static_cast<int8_t>(z));
                StepFacing(mob, mob->stepCount) = facing;
                StepGraphic(mob, mob->stepCount) = static_cast<uint16_t>(graphic);
                StepStatus(mob, mob->stepCount) = static_cast<uint8_t>(flags);
                mob->stepCount = mob->stepCount + 1;
            }
        }

        // follow target: walk to it when it is farther than 3 tiles
        CPlayer *player = reinterpret_cast<CPlayer *>(g_player);
        if (player->unk_1f8 != 0 && mob->serial == player->unk_1fc) {
            if (mob->IsInRange(3) == 0) {   // 0x00452f20
                uint32_t goalXY;
                int16_t goalZ;
                if (mob->stepCount > 0) {
                    int last = mob->stepCount - 1;
                    goalXY = static_cast<uint32_t>(static_cast<uint16_t>(StepX(mob, last))) |
                             (static_cast<uint32_t>(static_cast<uint16_t>(StepY(mob, last))) << 16);
                    goalZ = StepZ(mob, last);
                } else {
                    goalXY = static_cast<uint32_t>(static_cast<uint16_t>(mob->x)) |
                             (static_cast<uint32_t>(static_cast<uint16_t>(mob->y)) << 16);
                    goalZ = mob->z;
                }
                if (g_pathActive != 0)
                    PathSearch_FreeLists(g_pathSearch);   // 0x00466120
                PathNode *node = PathSearch_Run(g_pathSearch, player->x, player->y, player->z,
                                                static_cast<int16_t>(goalXY), static_cast<int16_t>(goalXY >> 16), goalZ,
                                                0x64, 1);   // 0x00466160
                g_pathCurrentNode = node;
                if (node == nullptr) {
                    Journal_Say(g_msgTarget, 0, 3, "Can't get there (follow)");   // 0x00479480
                    g_pathActive = 0;
                    return;
                }
                g_pathActive = 1;
                Path_ReverseLinks();   // 0x00445080
                while (node->parent != nullptr)
                    node = node->parent;
                g_pathCurrentNode = node;
                g_pathX = node->x;
                g_pathY = node->y;
                g_pathZ = node->z;
                g_pathStartNode = node;
                g_pathFacing = reinterpret_cast<CObj *>(g_player)->facing;
            }
        }
    }

    if (isNew == 1)
        FlushLabels(obj);   // 0x00457230
    if (obj->IsInRange(0x12) == 0) {   // 0x00452f20
        if (decomp::vcall(PORT_RET(int), obj, 0x64) == 0)
            decomp::vcall_void(obj, 4);
    }
}
HOOK_METHOD(0x00455760, CObjectManager, AddOrUpdateObject);
