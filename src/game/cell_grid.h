// CCellGrid: the 64 x 64 window of world cells (one static instance at 0x0069ffe8; its
// cell table at +0x10 is the bucket table g_tileBuckets at 0x0069fff8, see obj.h). Each cell
// heads a doubly linked list of CWorldObj (next at +0x14, prev at +0x18).
// CWorldPicker: the screen-point picker (one static instance at 0x0069e0c0).
// Layouts from the assembly of decomp/merged/chunk_09_0042caf0.cpp (0x0042fa60 ..).
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "world_obj.h"
#include "globals.h"

class CCellGrid {
public:
    // 0x0042fa60  size 0x23  clear the cell table and set the window origin to (-128, -128).
    CCellGrid *__thiscall Ctor();
    // 0x0042fa90  size 0x22  purge every cell.
    void __thiscall PurgeAll();
    // 0x0042fac0  size 0xa3  unlink and release the objects of cell (x, y) that fail the virtual
    // tests (slots 0x2c / 0x1c). RET 8.
    void __thiscall PurgeCell(int x, int y);
    // 0x0042fb70  size 0x1d8  scroll the window to the view centre (0x0068dbbc / 0x0068dbc0),
    // purging the rows and columns that wrapped.
    void __thiscall ScrollToView();

    int32_t cellX;          // +0x00  window origin index (originX & 0x3f)
    int32_t cellY;          // +0x04
    int32_t originX;        // +0x08  window origin in world tiles
    int32_t originY;        // +0x0c
    CWorldObj *cells[0x1000];   // +0x10
};

static_assert(offsetof(CCellGrid, cellX) == 0x00, "CCellGrid::cellX");
static_assert(offsetof(CCellGrid, cellY) == 0x04, "CCellGrid::cellY");
static_assert(offsetof(CCellGrid, originX) == 0x08, "CCellGrid::originX");
static_assert(offsetof(CCellGrid, originY) == 0x0c, "CCellGrid::originY");
static_assert(offsetof(CCellGrid, cells) == 0x10, "CCellGrid::cells");
static_assert(sizeof(CCellGrid) == 0x4010, "CCellGrid size");

#define g_cellGrid GLOBAL_AS(CCellGrid, g_cellGrid_storage)

class CWorldPicker {
public:
    // 0x0042fd50  size 0x92c  topmost pickable object under screen point (x, y), or 0.
    // `flag` != 0 skips terrain tiles. RET 0xc.
    CWorldObj *__thiscall GetObjectAt(int x, int y, int flag);

    int32_t zFilter;        // +0x00  nonzero: terrain above maxZ is skipped
    int32_t maxZ;           // +0x04
};

static_assert(offsetof(CWorldPicker, maxZ) == 0x04, "CWorldPicker::maxZ");

#define g_worldPicker GLOBAL_AS(CWorldPicker, g_worldPicker_storage)

// 0x00430680  size 0x101  terrain tile under screen point (x, y) whose rect centre is nearest
// (Manhattan distance), or 0.
CWorldObj *__stdcall CellGrid_GetNearestTerrainAt(int x, int y);
// 0x00430790  size 0x37  walks the list of cell (x, y) to its end and does nothing else.
CWorldObj *__stdcall CellGrid_WalkCell(int x, int y);
// 0x004307d0  size 0xab  invalidate the terrain tiles whose map tile id matches `tile`.
void __stdcall CellGrid_InvalidateTerrainTile(uint32_t tile);
// 0x00430880  size 0x72  re-link the pickable non-mobile objects whose graphic + offset is `id`.
void __stdcall CellGrid_RelinkById(int id);
