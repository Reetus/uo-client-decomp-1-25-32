// CTerrainTile: one land tile of the world (a CWorldObj with four corner heights,
// per-vertex light levels and a chosen triangulation). Layout partial; see
// decomp/pass2/0x0042efa0.cpp. Size is not known yet (allocation site not ported).
#pragma once

#include <cstddef>
#include <cstdint>

#include "world_obj.h"

// Position argument passed to the position setter 0x0042eec0 (8 bytes by value).
struct TileCoords {
    int32_t a;      // x | y << 16
    int16_t b;      // z
};

class CTerrainTile : public CWorldObjLow {
public:
    virtual uint32_t __thiscall GetBaseZ();   // slot 0x48: 0x004488a0 (was MinOfFourBytes_004488a0)
    virtual int __thiscall Slot1c() { return 1; }
    virtual int __thiscall Slot20() { return 1; }
    // 0x0042efa0: set tile id, position and the four corner heights, compute the
    // per-vertex light levels from the neighbouring cells and choose the triangulation.
    void __thiscall Init(int tileId, TileCoords *coords, signed char *corners);
    // 0x00448810: height of the tile centre, the mean of the two corners of the flatter diagonal (0x0044fe30).
    virtual uint32_t __thiscall GetTopZ();   // slot 0x44
    // 0x00448870  RET 4: height at the corner (odd i) or edge midpoint (even i) number i of the eight directions.
    virtual uint16_t __thiscall GetHeightAt(int i);   // slot 0x4c

    uint8_t pad_34[4];
    int32_t tileId;             // +0x38
    int32_t lightLevel[4];      // +0x3c  per-vertex light 0..15
    int32_t triangle[6];        // +0x4c  six vertex indices of the two triangles
    int32_t diag0;              // +0x64  chosen diagonal corner
    int32_t diag1;              // +0x68
    int8_t height[4];           // +0x6c  corner heights (signed)
    int32_t flat;               // +0x70  1 when the tile is flat enough
};

static_assert(offsetof(CTerrainTile, tileId) == 0x38, "CTerrainTile::tileId");
static_assert(offsetof(CTerrainTile, lightLevel) == 0x3c, "CTerrainTile::lightLevel");
static_assert(offsetof(CTerrainTile, triangle) == 0x4c, "CTerrainTile::triangle");
static_assert(offsetof(CTerrainTile, diag0) == 0x64, "CTerrainTile::diag0");
static_assert(offsetof(CTerrainTile, diag1) == 0x68, "CTerrainTile::diag1");
static_assert(offsetof(CTerrainTile, height) == 0x6c, "CTerrainTile::height");
static_assert(offsetof(CTerrainTile, flat) == 0x70, "CTerrainTile::flat");
