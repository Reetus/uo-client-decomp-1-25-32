// CMap: the world map / tile-block view object (one static instance at
// 0x0068dbb8). Partial layout from decomp/pass2/0x00445460.cpp. The view
// origin (+4, +8) is also read by other code as DAT_0068dbbc / DAT_0068dbc0.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

class CMap {
public:
    int32_t viewX;          // +0x004  view origin x (DAT_0068dbbc)
    int32_t viewY;          // +0x008  view origin y (DAT_0068dbc0)
    int32_t viewZ;          // +0x00c
    int32_t mapId;          // +0x010
    uint8_t pad_14[0x100];
    int32_t width;          // +0x114  map width in cells (wraps)
    int32_t height;         // +0x118  map height in cells
    int32_t blkMinX;        // +0x11c  mapId-dependent block bounds, set by SetPos
    int32_t blkMaxX;        // +0x120
    int32_t blkMinY;        // +0x124
    int32_t blkMaxY;        // +0x128
    int32_t originX;        // +0x12c
    int32_t originY;        // +0x130
    int32_t extentX;        // +0x134
    int32_t extentY;        // +0x138
    int32_t blocksY;        // +0x13c  block count along y
    int32_t blocksX;        // +0x140  block count along x
    int32_t lastBlockX;     // +0x144  window origin of the last call
    int32_t lastBlockY;     // +0x148
    int32_t cache[0x24];    // +0x14c  indices (bx * blocksY + by) of the 6x6 loaded blocks, -1 when free

    // 0x00445460  size 0x2c3  stream the statics around the view centre (6x6 block window).
    void __thiscall StreamStatics();

    // Second pass, map_world_ops.cpp.
    // 0x00445e30  RET 0xc: move the view by (dx, dy), wrap, set z, restream when the player is linked.
    void __thiscall Move(int dx, int dy, int z);
    // 0x00445f00  RET 0x10: set the view position (x == -1 keeps it) and map id.
    void __thiscall SetPos(int x, int y, int z, int newMapId);
    // 0x00445fb0  RET 8: z byte of the cell (x, y), coordinates wrapped once.
    uint8_t __thiscall GetCellZ(int x, int y);
    // 0x00446030  RET 8: z of the four neighbour cells into g_NeighborZ4 (0x00528608), returns it.
    uint8_t *__thiscall GetNeighborZ(int x, int y);
    // 0x00446120  RET 8: tile id at (x, y), 0x8001 outside the map.
    uint32_t __thiscall GetTileId(int x, int y);   // the caller reads all of EAX

    // Second pass, map_world_cells.cpp.
    // 0x00445370: the compiler's vptr (table 0x0049f6f4), the map size (0x1800 x 0x1000), 0x200 x 0x300 blocks, mapId -1
    // and an empty cache.
    CMap();
    // The destructor only restores the vptr. Defined inline in map_world_cells.cpp so the scalar deleting destructor
    // (0x004453e0) expands it as the original's does.
    virtual ~CMap();
    // 0x00445420: forget the streamed window (blocks, cache, mapId) and purge the cell grid.
    void __thiscall Reset();
    // 0x00445730  RET 0xc: the 4x4 cells from (x - 1, y - 1) all have z byte `z` (cells outside the map count as flat).
    int __thiscall AreaFlatAt(int x, int y, int z);
    // 0x00445800  RET 8: create the land tile object of cell (x, y) (outside the 0x1400 .. 0x1800 half: tile 0x244).
    void __thiscall CreateLandTile(int x, int y);
    // 0x004459c0  RET 8: as CreateLandTile with (x, y) wrapped by the map size first.
    void __thiscall CreateLandTileWrapped(int x, int y);
    // 0x00445ba0: scroll the cell grid to the view, stream the statics, create the land tiles of the 0x25 x 0x25
    // window around the view that do not exist yet, redraw the radar and update the z filter of the picker.
    void __thiscall RefreshView();
};

static_assert(offsetof(CMap, viewX) == 0x04, "CMap::viewX");
static_assert(offsetof(CMap, viewY) == 0x08, "CMap::viewY");
static_assert(offsetof(CMap, viewZ) == 0x0c, "CMap::viewZ");
static_assert(offsetof(CMap, mapId) == 0x10, "CMap::mapId");
static_assert(offsetof(CMap, width) == 0x114, "CMap::width");
static_assert(offsetof(CMap, height) == 0x118, "CMap::height");
static_assert(offsetof(CMap, blkMinX) == 0x11c, "CMap::blkMinX");
static_assert(offsetof(CMap, blkMaxX) == 0x120, "CMap::blkMaxX");
static_assert(offsetof(CMap, blkMinY) == 0x124, "CMap::blkMinY");
static_assert(offsetof(CMap, blkMaxY) == 0x128, "CMap::blkMaxY");
static_assert(offsetof(CMap, originX) == 0x12c, "CMap::originX");
static_assert(offsetof(CMap, extentY) == 0x138, "CMap::extentY");
static_assert(offsetof(CMap, blocksY) == 0x13c, "CMap::blocksY");
static_assert(offsetof(CMap, blocksX) == 0x140, "CMap::blocksX");
static_assert(offsetof(CMap, lastBlockX) == 0x144, "CMap::lastBlockX");
static_assert(offsetof(CMap, lastBlockY) == 0x148, "CMap::lastBlockY");
static_assert(offsetof(CMap, cache) == 0x14c, "CMap::cache");

#define g_map GLOBAL_AS(CMap, g_map_storage)

// 0x00445300  size 0x6f  Chebyshev-style distance between the point (x, y) at `a` and (bx, by).
// ECX = a, EDX = bx, stack = by (RET 4). 9999 when the two points are on different map halves.
int __fastcall MapDistance(const int16_t *a, int bx, int by);
