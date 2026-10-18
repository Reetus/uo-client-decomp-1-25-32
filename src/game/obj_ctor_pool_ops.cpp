// Pooled world objects: the land tile pool (vtable 0x0049f788), the pool of vtable 0x0049f730, and the
// constructors / pool release of the CObj (0x78 bytes) and CDynamicObj (0xa8 bytes) classes.
// 0x004519c0 .. 0x00451d80 (porting pass 2, batch 20). The SEH frames of the original are omitted.
#include <new>

#include "obj.h"
#include "obj_inline.h"

#include "../runtime/ported.h"
#include "crt.h"
#include "vcall.h"
#include "world_obj_ops.h"
#include "vtables.h"
#include "globals.h"

// Pool and list globals (Ghidra addresses).

// ---------------------------------------------------------------------------
// Land tile pool
// ---------------------------------------------------------------------------

// 0x004519c0  size 0xf9
// Pool "new" of a land tile, then CTerrainTile::Init(tileId, coords, corners). ECX = tile id, EDX = coords,
// the corner heights on the stack (RET 4). An empty free list gets a slab of 0x80 tiles (operator new 0x3a04:
// count cookie + 0x80 * 0x74) vector-constructed with 0x00451b80 / 0x00451ba0 and threaded onto the free list
// through +0x14 (slots 0x7f down to 1; slot 0 is used). The SEH frame is omitted.
CTerrainTile *__fastcall CTerrainTile_PoolNew(int tileId, TileCoords *coords, signed char *corners)
{
    CTerrainTileOps *tile;

    if (g_terrFree != nullptr) {
        tile = g_terrFree;
        g_terrFree = reinterpret_cast<CTerrainTileOps *>(tile->next);
    } else {
        g_terrBuilding = 1;
        char *base = reinterpret_cast<char *>(new CTerrainTileOps[0x80]);   // operator new 0x3a04: count cookie + 0x80 * 0x74
        tile = reinterpret_cast<CTerrainTileOps *>(base);
        g_terrTotal = g_terrTotal + 0x80;
        char *prevFree = reinterpret_cast<char *>(g_terrFree);   // reloaded after the call, still 0 here
        g_terrBuilding = 0;
        char *slot = base + 0x398c;   // slot 0x7f
        int n = 0x7f;
        do {
            *reinterpret_cast<char **>(slot + 0x14) = prevFree;
            prevFree = slot;
            slot = slot - 0x74;
            n--;
        } while (n != 0);
        g_terrFree = reinterpret_cast<CTerrainTileOps *>(prevFree);
    }
    g_terrLive = g_terrLive + 1;
    tile->Init(tileId, coords, corners);   // 0x0042efa0
    return tile;
}
HOOK_FUNC(0x004519c0, CTerrainTile_PoolNew);

// 0x00451b80  size 0x12
CTerrainTileOps::CTerrainTileOps()
{
}
HOOK_CTOR(0x00451b80, CTerrainTileOps, ());

// 0x00451ba0  size 0xb
CTerrainTileOps::~CTerrainTileOps()
{
}
HOOK_DTOR(0x00451ba0, CTerrainTileOps);
HOOK_VTABLE(0x0049f788, CTerrainTileOps);
HOOK_VDD(0x00451ac0, CTerrainTileOps);

// 0x00451bb0  size 0x2f
// Give the tile back to the pool: leave the world (virtual slot 0x14) and push onto the free list.
void __thiscall CTerrainTileOps::DeleteSelf()
{
    g_terrLive = g_terrLive - 1;
    if (inWorld != 0)
        decomp::vcall_void(this, 0x14);
    next = g_terrFree;
    g_terrFree = this;
}
HOOK_VMETHOD(0x00451bb0, CTerrainTileOps, DeleteSelf);

// ---------------------------------------------------------------------------
// The pool of vtable 0x0049f730
// ---------------------------------------------------------------------------

// 0x00451b20  size 0x12
CObjVt49f730::CObjVt49f730()
{
}
HOOK_CTOR(0x00451b20, CObjVt49f730, ());

// 0x00451b40  size 0xb
CObjVt49f730::~CObjVt49f730()
{
}
HOOK_DTOR(0x00451b40, CObjVt49f730);
HOOK_VTABLE(0x0049f730, CObjVt49f730);
HOOK_VDD(0x00451960, CObjVt49f730);

// 0x00451b50  size 0x2f
void __thiscall CObjVt49f730::DeleteSelf()
{
    g_obj730Live = g_obj730Live - 1;
    if (inWorld != 0)
        decomp::vcall_void(this, 0x14);
    next = g_obj730Free;
    g_obj730Free = this;
}
HOOK_VMETHOD(0x00451b50, CObjVt49f730, DeleteSelf);

// ---------------------------------------------------------------------------
// CObj / CDynamicObj constructors and pool release
// ---------------------------------------------------------------------------

// Constructs a CObj in place (for callers that still hold raw memory and call the old Ctor).
CObj *__thiscall CObj::Ctor()
{
    new (this) CObj;
    return this;
}

// 0x00451d10  size 0x69
// Give the object back to the pool: leave the world when linked (virtual slot 0x50), push onto the free list
// (next at +0x40), delete every bark gump at +0x68, and drop the +0x60/+0x64 list membership when virtual
// slot 0x64 says so.
void __thiscall CObj::DeleteSelf()
{
    g_poolLive = g_poolLive - 1;
    if (unlinked == 0)
        decomp::vcall_void(this, 0x50);
    link40 = reinterpret_cast<CObj *>(g_poolFree);
    g_poolFree = reinterpret_cast<char *>(this);
    g_childDeleteFlag = 1;
    while (barkList != nullptr)
        decomp::vdelete(barkList);
    g_childDeleteFlag = 0;
    if (decomp::vcall(PORT_RET(int), this, 0x64) != 0)
        UnlinkList70();   // 0x00452250
}
HOOK_VMETHOD(0x00451d10, CObj, DeleteSelf);

// 0x00451d80  size 0xe2
// Constructor of the 0xa8-byte class: the inline CObj constructor, vtable
// 0x0049f860, then (unless the dynamic pool slab is being constructed, DAT_00721054) push onto the global
// list headed at 0x0068ce7c and clear the item fields.
CDynamicObj::CDynamicObj()
{
    if (g_dynPoolBuilding == 0) {
        CDynamicObj *oldHead = g_dynListHead;
        listNext = oldHead;
        if (oldHead != nullptr)
            oldHead->listPrev = this;
        CDynamicObj *oldTail = g_dynListTail;
        g_dynListHead = this;
        if (oldTail == nullptr)
            g_dynListTail = this;
        listPrev = nullptr;
        container = nullptr;
        gump8c = nullptr;
        gump88 = nullptr;
        *reinterpret_cast<int32_t *>(reinterpret_cast<char *>(this) + 0x90) = 0;
        light = 0;
        partList = nullptr;
        buf_a0 = nullptr;
        unk_a4 = 0;
        flags98 = 0;
    }
}
HOOK_CTOR(0x00451d80, CDynamicObj, ());

// Constructs a CDynamicObj in place (for callers that still hold raw memory and call the old Ctor).
CDynamicObj *__thiscall CDynamicObj::Ctor()
{
    new (this) CDynamicObj;
    return this;
}

