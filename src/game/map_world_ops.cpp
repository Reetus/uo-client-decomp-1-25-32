// CMap second pass: view movement, cell lookups, line of sight, and the two tiny allocator forwarders at
// 0x004462f0 / 0x00446310. Procedures 0x00445e30 .. 0x00446310 of chunk_14_00445800.
#include "pos3.h"
#include "map_world.h"
#include "map_world_ops_ports.h"

#include "../runtime/ported.h"
#include "file_manager.h"
#include "globals.h"
#include "gump_manager.h"
#include "obj.h"
#include "object_manager.h"
#include "crt.h"
#include "map_world_cells_ports.h"
#include "dirty_rects_ports.h"
#include "class_0049c690.h"

#define g_worldObjects GLOBAL_PTR(void, g_objMgr_storage)       // object manager (world lists)

// 0x00445db0: shift every object of the world cell buckets by (ECX, EDX) and refresh it.
#define Map_Rebase(a1, a2) (Map_RebaseCells(a1, a2))
// 0x0044fdf0: invalidate the whole screen; ECX is not read.
#define Screen_InvalidateAll() (InvalidateAll())
// 0x00406720: empty procedure (ported as CWorldObj::ToVoid).
#define Nop_00406720 PORT_FUNC0(&Nop_00406720, void, __cdecl)

// 0x00445e30  size 0xc3
// Move the view origin by (dx, dy). When the origin is on the x >= 0x1400 half there is no wrapping; otherwise
// x wraps at 0x1400 and y at 0x1000, and each wrap rebases the world cell buckets with the opposite offset.
// Then store z and, when the player is linked into the world, restream (0x00445ba0).
void __thiscall CMap::Move(int dx, int dy, int z)
{
    if (viewX >= 0x1400) {
        viewX = dx + viewX;
        viewY = viewY + dy;
    } else {
        viewX = viewX + dx;
        viewY = viewY + dy;
        if (viewX >= 0x1400) {
            viewX = viewX - 0x1400;
            Map_Rebase(-0x1400, 0);
        }
        if (viewX < 0) {
            viewX = viewX + 0x1400;
            Map_Rebase(0x1400, 0);
        }
        if (viewY >= 0x1000) {
            viewY = viewY - 0x1000;
            Map_Rebase(0, -0x1000);
        }
        if (viewY < 0) {
            viewY = viewY + 0x1000;
            Map_Rebase(0, 0x1000);
        }
    }
    viewZ = z;
    if (g_player->unlinked == 0)
        this->CMap::RefreshView();
}
HOOK_METHOD(0x00445e30, CMap, Move);

// 0x00445f00  size 0xa4
// Set the view position (x == -1 leaves it alone) and the map id; a changed map id recomputes the block bounds.
void __thiscall CMap::SetPos(int x, int y, int z, int newMapId)
{
    Screen_InvalidateAll();
    if (x != -1) {
        viewX = x;
        viewY = y;
        viewZ = z;
    }
    if (newMapId != mapId) {
        int ox = originX;
        mapId = newMapId;
        blkMinX = ox >> 3;
        int oy = originY;
        blkMinY = oy >> 3;
        blkMaxX = (extentX + ox) >> 3;
        blkMaxY = (extentY + oy) >> 3;
        Nop_00406720();
    }
    if (g_player->unlinked == 0 && x != -1) {
        this->CMap::RefreshView();
        g_gumpMgr.DeleteMissileGumps();
    }
}
HOOK_METHOD(0x00445f00, CMap, SetPos);

// 0x00445fb0  size 0x71
// z byte of cell (x, y): coordinates wrap once by the map size, then the 8x8 block record (index
// (x >> 3) * blocksY + (y >> 3)) holds 3-byte cells {id.w, z.b} from +4.
uint8_t __thiscall CMap::GetCellZ(int x, int y)
{
    if (x < 0)
        x += width;
    if (x >= width)
        x -= width;
    if (y < 0)
        y += height;
    if (y >= height)
        y -= height;
    uint8_t *rec = g_fileManager.GetRecordPtr(0, (x >> 3) * blocksY + (y >> 3));
    return rec[(y & 7) * 3 * 8 + (x & 7) * 3 + 6];
}
HOOK_METHOD(0x00445fb0, CMap, GetCellZ);

// 0x00446030  size 0xe3
// z of the four cells around (x, y) (offsets in the tables at 0x004a8e08 / 0x004a8e18) into g_NeighborZ4;
// outside the map the first dword of it is cleared. Returns g_NeighborZ4.
uint8_t *__thiscall CMap::GetNeighborZ(int x, int y)
{
    if (x >= 0 && y >= 0 && x < width && y < height) {
        int i = 0;
        do {
            int cy = g_NeighborDY[i] + y;
            int cx = g_NeighborDX[i] + x;
            if (cx < 0)
                cx += width;
            if (cx >= width)
                cx -= width;
            if (cy < 0)
                cy += height;
            if (cy >= height)
                cy -= height;
            uint8_t *rec = g_fileManager.GetRecordPtr(0, (cx >> 3) * blocksY + (cy >> 3));
            g_NeighborZ4[i] = rec[((cx & 7) + (cy & 7) * 8 + 2) * 3];
            i++;
        } while (i < 4);
        return g_NeighborZ4;
    }
    *reinterpret_cast<uint32_t *>(g_NeighborZ4) = 0;
    return g_NeighborZ4;
}
HOOK_METHOD(0x00446030, CMap, GetNeighborZ);

// 0x00446120  size 0x67
// Tile id (u16 at +4 of the 3-byte cell) at (x, y); 0x8001 outside the map.
uint32_t __thiscall CMap::GetTileId(int x, int y)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return 0x8001;
    uint8_t *rec = g_fileManager.GetRecordPtr(0, (x >> 3) * blocksY + (y >> 3));
    return *reinterpret_cast<uint16_t *>(rec + ((x & 7) + (y & 7) * 8) * 3 + 4);
}
HOOK_METHOD(0x00446120, CMap, GetTileId);

// 0x00446190  size 0x157
// Line of sight: walk from `from` to `to` in 16.16 fixed point, one step per cell of the longer axis (z is
// interpolated by the same step count), and fail when an object found at an intermediate point has any of the
// `mask` bits in its vtable slot 0x40 flags. The SEH frame is omitted. RET 0x14: two 8-byte coordinates and mask.
int __stdcall Map_LineOfSightClear(Pos3 from, Pos3 to, uint32_t mask)
{
    if (from.x == to.x && from.y == to.y && from.z == to.z)
        return 1;

    int sx = static_cast<int16_t>(from.x);
    int ex = static_cast<int16_t>(to.x);
    int sy = static_cast<int16_t>(from.y);
    int ey = static_cast<int16_t>(to.y);
    int sz = static_cast<int16_t>(from.z);
    int dy = ey - sy;
    int dx = ex - sx;
    int dz = static_cast<int16_t>(to.z) - sz;
    int steps = abs(dx);
    int ady = abs(dy);
    if (ady > steps)
        steps = ady;
    if (steps == 0)
        return 1;

    dx = (dx << 16) / steps;
    dy = (dy << 16) / steps;
    dz = (dz << 16) / steps;
    int fx = sx << 16;
    int fy = sy << 16;
    int fz = sz << 16;
    Pos3 c;
    int count = steps - 1;
    for (int i = 0; i < count; i++) {
        fx += dx;
        fy += dy;
        fz += dz;
        c.x = static_cast<int16_t>(fx >> 16);
        c.y = static_cast<int16_t>(fy >> 16);
        c.z = static_cast<int16_t>(fz >> 16);
        CObj *obj = reinterpret_cast<CObjectManager *>(g_worldObjects)->FindAtCoords(c);
        if (obj != nullptr) {
            uint32_t flags = PORT_VSLOT0(obj, 0x40, uint32_t)();
            if ((mask & flags) != 0)
                return 0;
        }
    }
    return 1;
}
HOOK_FUNC(0x00446190, Map_LineOfSightClear);

// 0x004462f0  size 0x13
// operator new forwarder (ECX = allocator object, not read): (size, name); name is not read. RET 8.
void *__stdcall Map_Alloc(size_t size, const char *name)
{
    (void)name;
    return ::operator new(size);   // 0x0048ad70
}
HOOK_FUNC(0x004462f0, Map_Alloc);

// 0x00446310  size 0x13
// operator delete forwarder (ECX not read): (pointer, second argument not read). RET 8.
void __stdcall Map_Free(void *p, int unused)
{
    (void)unused;
    operator delete(p);   // 0x0048ad60
}
HOOK_FUNC(0x00446310, Map_Free);
