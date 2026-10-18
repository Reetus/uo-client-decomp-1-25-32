// Pool allocation and compiler-generated deleting destructors of the world object classes
// (second pass, batch 19): CObj (0x78 bytes), CDynamicObj (0xa8) and the 0x3c-byte terrain tile.
// The SEH frames of the original are omitted. Companion of obj.cpp (0x004514e0 CObj pool).
#include "obj.h"

#include "../runtime/ported.h"
#include "crt.h"
#include "raw_field.h"
#include "terrain_tile.h"
#include "terrain_tile_draw_ports.h"
#include "obj_pool_alloc_ports.h"
#include "globals.h"

// Pool of CDynamicObj slabs (0x40 objects each, count cookie in front).

// Pool of 0x3c-byte terrain tiles (0x80 per slab).
#define pool_tileFree GLOBAL_AS(char *, g_obj730Free)       // free list, next pointer at +0x14

// 0x004516a0  size 0x15e
// Take a CDynamicObj from the pool (a new slab of 0x40, vector-constructed with 0x00451d80, when the free
// list is empty), link it at the head of the item list and initialise it: ECX = serial (+0x78), EDX =
// graphic (&0x3fff, +0x38), stack byte = +0x3a. RET 4.
CDynamicObj *__fastcall DynamicObj_PoolNew(uint32_t serial, uint32_t graphic, uint32_t param)
{
    char *obj;
    uint32_t *slab;
    char *prevFree;
    char *slot;
    char *tail;
    int n;
    CDynamicObj *o;
    CDynamicObj *head;

    if (g_pool_dynFree == nullptr) {
        g_dynPoolBuilding = 1;
        obj = reinterpret_cast<char *>(new CDynamicObj[0x40]);   // operator new 0x2a04: count cookie + 0x40 * 0xa8
        pool_dynTotal = pool_dynTotal + 0x40;
        prevFree = g_pool_dynFree;
        g_dynPoolBuilding = 0;
        slot = obj + 0x2958;   // slot 0x3f
        n = 0x3f;
        do {
            *reinterpret_cast<char **>(slot + 0x80) = prevFree;
            prevFree = slot;
            slot = slot - 0xa8;
            n--;
        } while (n != 0);
        g_pool_dynFree = prevFree;
    } else {
        obj = g_pool_dynFree;
        g_pool_dynFree = *reinterpret_cast<char **>(obj + 0x80);
    }
    g_pool_dynLive = g_pool_dynLive + 1;
    o = reinterpret_cast<CDynamicObj *>(obj);
    head = g_dynListHead;
    o->listNext = head;
    if (head != nullptr)
        head->listPrev = o;
    tail = reinterpret_cast<char *>(g_dynListTail);
    g_dynListHead = o;
    if (tail == nullptr)
        g_dynListTail = o;
    o->listPrev = nullptr;
    o->container = nullptr;
    o->gump8c = nullptr;
    o->gump88 = nullptr;
    RAWF(int32_t, o, 0x90) = 0;
    o->light = 0;
    o->partList = nullptr;
    o->buf_a0 = nullptr;
    o->unk_a4 = 0;
    o->flags98 = 0;
    o->serial = static_cast<int32_t>(serial);
    o->graphic = static_cast<uint16_t>(graphic & 0x3fff);
    o->unk_3a = static_cast<uint8_t>(param);
    return o;
}
HOOK_FUNC(0x004516a0, DynamicObj_PoolNew);

// 0x00451860  size 0xf5
// Take a 0x3c-byte terrain tile from the pool (a new slab of 0x80, vector-constructed with 0x00451b20,
// when the free list is empty) and set its tile id and position with 0x0042ee90: ECX = tile id, EDX = the
// position {x | y << 16, z}.
CTerrainTile *__fastcall TerrainTile_PoolNew(int tileId, TileCoords *coords)
{
    char *tile;
    uint32_t *slab;
    char *prevFree;
    char *slot;
    int n;

    if (pool_tileFree == nullptr) {
        pool_tileBuilding = 1;
        tile = reinterpret_cast<char *>(new CObjVt49f730[0x80]);   // operator new 0x1e04: count cookie + 0x80 * 0x3c
        pool_tileTotal = pool_tileTotal + 0x80;
        prevFree = pool_tileFree;
        pool_tileBuilding = 0;
        slot = tile + 0x1dc4;   // slot 0x7f
        n = 0x7f;
        do {
            *reinterpret_cast<char **>(slot + 0x14) = prevFree;
            prevFree = slot;
            slot = slot - 0x3c;
            n--;
        } while (n != 0);
        pool_tileFree = prevFree;
    } else {
        tile = pool_tileFree;
        pool_tileFree = *reinterpret_cast<char **>(tile + 0x14);
    }
    g_obj730Live = g_obj730Live + 1;
    PORT_THISCALL2(&TerrainTile_SetTileAndPos, void, tile, int, TileCoords *)(tileId, coords);   // TerrainTile_SetTileAndPos, RET 8
    return reinterpret_cast<CTerrainTile *>(tile);
}
HOOK_FUNC(0x00451860, TerrainTile_PoolNew);

