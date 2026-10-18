// CMap, second pass, part 2: constructor, reset, land tile creation for the cells around the view, the view refresh,
// and the cell bucket rebase. 0x00445370 .. 0x00445db0. The SEH frames of the original are omitted.
#include "map_world.h"

#include <cstring>

#include "../runtime/ported.h"
#include "cell_grid.h"
#include "file_manager.h"
#include "globals.h"
#include "crt.h"
#include "radar_gump.h"
#include "raw_field.h"
#include "vcall.h"
#include "dirty_rects_ports.h"
#include "obj.h"
#include "obj_pool_alloc_ports.h"
#include "pos3.h"
#include "vtables.h"

// 0x0044fdf0: invalidate the whole screen; ECX is not read.
#define MC_InvalidateAll() (InvalidateAll())
// 0x004519c0: land tile from the pool (ECX = tile id, EDX = {x, y, z} words, stack = the four corner z bytes), RET 4.
#define MC_NewTileCorners(a1, a2, a3) ((void *)CTerrainTile_PoolNew(a1, (TileCoords *)(a2), (signed char *)(a3)))
// 0x00451860: land tile from the pool for a flat cell (ECX = tile id, EDX = {x, y, z} words).
#define MC_NewTileFlat(a1, a2) ((void *)TerrainTile_PoolNew(a1, (TileCoords *)(a2)))

// 0x00445370  size 0x68
CMap::CMap()
{
    viewZ = 0;
    viewY = 0;
    viewX = 0;
    lastBlockY = static_cast<int32_t>(0xffffd8f1);
    lastBlockX = static_cast<int32_t>(0xffffd8f1);
    width = 0x1800;
    height = 0x1000;
    blocksX = 0x300;
    blocksY = 0x200;
    mapId = -1;
    for (int i = 0; i < 0x24; i++)
        cache[i] = -1;
}
HOOK_CTOR(0x00445370, CMap, ());

inline CMap::~CMap()
{
}
HOOK_VTABLE(0x0049f6f4, CMap);
HOOK_SDD(0x004453e0, CMap);   // slot 0

// 0x00445410  size 0x7
// The destructor as the atexit thunk 0x0044aac0 jumps to it (ECX = g_Map): only the vptr store. A plain
// function that expands the inline destructor.
void __fastcall MapData_SetVtable_00445410(void *self, void *)
{
    reinterpret_cast<CMap *>(self)->CMap::~CMap();
}
HOOK_FUNC(0x00445410, MapData_SetVtable_00445410);

// 0x00445420  size 0x33
void __thiscall CMap::Reset()
{
    lastBlockY = static_cast<int32_t>(0xffffd8f1);
    lastBlockX = static_cast<int32_t>(0xffffd8f1);
    for (int i = 0; i < 0x24; i++)
        cache[i] = -1;
    mapId = -1;
    g_cellGrid.PurgeAll();   // 0x0042fa90
}
HOOK_METHOD(0x00445420, CMap, Reset);

// 0x00445730  size 0xc1
// The 4x4 cells from (x - 1, y - 1) to (x + 2, y + 2): 0 as soon as one z byte differs from `z`; a cell outside the
// map ends the scan with 1.
int __thiscall CMap::AreaFlatAt(int x, int y, int z)
{
    int cx = x - 1;
    int endX = x + 2;
    for (; cx <= endX; cx++) {
        for (int cy = y - 1; cy <= y + 2; cy++) {
            if (cx < 0 || cx >= width || cy < 0 || cy >= height)
                return 1;
            uint8_t *rec = g_fileManager.GetRecordPtr(0, (cx >> 3) * blocksY + (cy >> 3));   // 0x00440480
            int8_t cellZ = static_cast<int8_t>(rec[((cx & 7) + (cy & 7) * 8 + 2) * 3]);
            if (cellZ != z)
                return 0;
        }
    }
    return 1;
}
HOOK_METHOD(0x00445730, CMap, AreaFlatAt);


// 0x00445800  size 0x1c0
// Create the land tile object of cell (x, y). Inside the 0x1400 .. 0x1800 x 0 .. 0x1000 half the tile id is the word
// at +4 of the cell record (0x8001 outside the map size), elsewhere 0x244; bit 15 is dropped. The z bytes of the four
// neighbour cells are collected with GetCellZ. A tile whose flag word in the tile table is set and whose neighbours
// are all at one height (and whose 4x4 surroundings are flat) is created flat, the others with the corner heights
// and the mean as z. The new object is then asked to link itself (vtable slot 0x10).
void __thiscall CMap::CreateLandTile(int x, int y)
{
    Pos3 pos;
    int tile;
    if (x >= 0x1400 && x < 0x1800 && y >= 0 && y < 0x1000) {
        if (x >= 0 && x < width && y < height) {
            uint8_t *rec = g_fileManager.GetRecordPtr(0, (x >> 3) * blocksY + (y >> 3));   // 0x00440480
            tile = RAWF(uint16_t, rec, ((x & 7) + (y & 7) * 8) * 3 + 4);
        } else {
            tile = 0x8001;
        }
    } else {
        tile = 0x244;
    }
    tile &= 0x7fff;
    if (x >= 0 && y >= 0 && x < width && y < height) {
        for (int i = 0; i < 4; i++)
            g_NeighborZ4[i] = GetCellZ(x + g_NeighborDX[i], y + g_NeighborDY[i]);   // 0x00445fb0
    } else {
        *reinterpret_cast<uint32_t *>(g_NeighborZ4) = 0;
    }

    void *obj;
    if (RAWF(uint16_t, g_landTable, tile * 0x1c + 4) != 0 &&
        (g_NeighborZ4[0] != g_NeighborZ4[1] || g_NeighborZ4[1] != g_NeighborZ4[2] ||
         g_NeighborZ4[2] != g_NeighborZ4[3] ||
         AreaFlatAt(x, y, static_cast<int8_t>(g_NeighborZ4[0])) == 0)) {   // 0x00445730
        int avg = (static_cast<int8_t>(g_NeighborZ4[1]) + static_cast<int8_t>(g_NeighborZ4[3]) +
                   static_cast<int8_t>(g_NeighborZ4[2]) + static_cast<int8_t>(g_NeighborZ4[0])) >> 2;
        pos.x = static_cast<int16_t>(x);
        pos.y = static_cast<int16_t>(y);
        pos.z = static_cast<int16_t>(avg);
        obj = MC_NewTileCorners(tile, &pos, g_NeighborZ4);   // 0x004519c0
    } else {
        pos.x = static_cast<int16_t>(x);
        pos.y = static_cast<int16_t>(y);
        pos.z = static_cast<int16_t>(static_cast<int8_t>(g_NeighborZ4[0]));
        obj = MC_NewTileFlat(tile, &pos);   // 0x00451860
    }
    decomp::vcall_void(obj, 0x10);
}
HOOK_METHOD(0x00445800, CMap, CreateLandTile);

// 0x004459c0  size 0x1d7
// As CreateLandTile, but (x, y) are first wrapped once by 0x1400 / 0x1000, every lookup uses the wrapped cell (no
// 0x1400 .. 0x1800 test: a cell outside the map size has tile 0x8001) and the new object keeps the unwrapped position.
void __thiscall CMap::CreateLandTileWrapped(int x, int y)
{
    Pos3 pos;
    int wx = x;
    int wy = y;
    if (wx < 0)
        wx += 0x1400;
    if (wx >= 0x1400)
        wx -= 0x1400;
    if (wy < 0)
        wy += 0x1000;
    if (wy >= 0x1000)
        wy -= 0x1000;
    int tile;
    if (wx >= 0 && wy >= 0 && wx < width && wy < height) {
        uint8_t *rec = g_fileManager.GetRecordPtr(0, (wx >> 3) * blocksY + (wy >> 3));   // 0x00440480
        tile = RAWF(uint16_t, rec, ((wx & 7) + (wy & 7) * 8) * 3 + 4);
    } else {
        tile = 0x8001;
    }
    tile &= 0x7fff;
    if (wx >= 0 && wy >= 0 && wx < width && wy < height) {
        for (int i = 0; i < 4; i++)
            g_NeighborZ4[i] = GetCellZ(wx + g_NeighborDX[i], wy + g_NeighborDY[i]);   // 0x00445fb0
    } else {
        *reinterpret_cast<uint32_t *>(g_NeighborZ4) = 0;
    }

    void *obj;
    if (RAWF(uint16_t, g_landTable, tile * 0x1c + 4) != 0 &&
        (g_NeighborZ4[0] != g_NeighborZ4[1] || g_NeighborZ4[1] != g_NeighborZ4[2] ||
         g_NeighborZ4[2] != g_NeighborZ4[3] ||
         AreaFlatAt(wx, wy, static_cast<int8_t>(g_NeighborZ4[0])) == 0)) {   // 0x00445730
        int avg = (static_cast<int8_t>(g_NeighborZ4[1]) + static_cast<int8_t>(g_NeighborZ4[3]) +
                   static_cast<int8_t>(g_NeighborZ4[2]) + static_cast<int8_t>(g_NeighborZ4[0])) >> 2;
        pos.x = static_cast<int16_t>(x);
        pos.y = static_cast<int16_t>(y);
        pos.z = static_cast<int16_t>(avg);
        obj = MC_NewTileCorners(tile, &pos, g_NeighborZ4);   // 0x004519c0
    } else {
        pos.x = static_cast<int16_t>(x);
        pos.y = static_cast<int16_t>(y);
        pos.z = static_cast<int16_t>(static_cast<int8_t>(g_NeighborZ4[0]));
        obj = MC_NewTileFlat(tile, &pos);   // 0x00451860
    }
    decomp::vcall_void(obj, 0x10);
}
HOOK_METHOD(0x004459c0, CMap, CreateLandTileWrapped);

// 0x00445ba0  size 0x20d
// Refresh after the view moved: scroll the cell grid, stream the statics, make sure every cell of the 0x25 x 0x25
// window around the view has a land tile (the cell bucket is searched for a tile object at that position), redraw the
// radar, and then compare the height of the tile under the view with viewZ + 0x10 to turn the picker's z filter
// (0x0069e0c0 / 0x0069e0c4) on, move it or off. Turning it on also clears the back buffer.
void __thiscall CMap::RefreshView()
{
    g_cellGrid.ScrollToView();   // 0x0042fb70
    StreamStatics();             // 0x00445460
    if (viewX >= 0x1400) {
        for (int cy = viewY - 0x12; cy <= viewY + 0x12; cy++) {
            for (int cx = viewX - 0x12; cx <= viewX + 0x12; cx++) {
                CWorldObj *node = g_cellGrid.cells[(cy & 0x3f) * 64 + (cx & 0x3f)];
                while (node != nullptr) {
                    if (decomp::vcall(PORT_RET(int), node, 0x1c) != 0 && node->x == cx && node->y == cy)
                        break;
                    node = node->next;
                }
                if (node == nullptr)
                    CreateLandTile(cx, cy);   // 0x00445800
            }
        }
    } else {
        for (int cy = viewY - 0x12; cy <= viewY + 0x12; cy++) {
            for (int cx = viewX - 0x12; cx <= viewX + 0x12; cx++) {
                CWorldObj *node = g_cellGrid.cells[(cy & 0x3f) * 64 + (cx & 0x3f)];
                while (node != nullptr) {
                    if (decomp::vcall(PORT_RET(int), node, 0x1c) != 0 && node->x == cx && node->y == cy)
                        break;
                    node = node->next;
                }
                if (node == nullptr)
                    CreateLandTileWrapped(cx, cy);   // 0x004459c0
            }
        }
    }
    if (g_radarGump != nullptr)
        g_radarGump->Redraw();   // 0x0046a4f0

    CWorldObj *under = g_cellGrid.cells[(viewY & 0x3f) * 64 + (viewX & 0x3f)];
    while (under != nullptr) {
        if (decomp::vcall(PORT_RET(int), under, 0x1c) != 0)
            break;
        under = under->next;
    }
    int groundZ = (under == nullptr) ? 0 : under->z;
    int limit = viewZ + 0x10;
    if (limit < groundZ && g_worldPicker.zFilter == 0) {
        memset(g_backBuffer, 0, 0x25800 * 4);
        g_worldPicker.zFilter = 1;
        g_worldPicker.maxZ = viewZ + 0x10;
        MC_InvalidateAll();   // 0x0044fdf0
    } else if (limit < groundZ && g_worldPicker.zFilter == 1) {
        if (g_worldPicker.maxZ != limit) {
            g_worldPicker.maxZ = limit;
            MC_InvalidateAll();
        }
    } else if (limit >= groundZ && g_worldPicker.zFilter == 1) {
        g_worldPicker.zFilter = 0;
        MC_InvalidateAll();
    }
}
HOOK_METHOD(0x00445ba0, CMap, RefreshView);

// 0x00445db0  size 0x7d
// ECX = dx, EDX = dy: shift every object of the 64 x 64 cell buckets by (dx, dy), then for each of them call vtable
// slots 0x14 and 0x10 (unlink / relink at the new position).
void __fastcall Map_RebaseCells(int dx, int dy)
{
    MC_InvalidateAll();   // 0x0044fdf0
    for (int row = 0; row < 0x1000; row += 0x40) {
        for (int x = 0; x < 0x40; x++) {
            for (CWorldObj *obj = g_cellGrid.cells[row + x]; obj != nullptr; obj = obj->next) {
                obj->x = static_cast<int16_t>(obj->x + dx);
                obj->y = static_cast<int16_t>(obj->y + dy);
            }
            for (CWorldObj *obj = g_cellGrid.cells[row + x]; obj != nullptr; obj = obj->next) {
                decomp::vcall_void(obj, 0x14);
                decomp::vcall_void(obj, 0x10);
            }
        }
    }
}
HOOK_FUNC(0x00445db0, Map_RebaseCells);
