// World view redraw: visible-cell table, CWorldRenderer, roof helpers (0x0042a350 .. 0x0042a4b0,
// 0x0042bd90 .. 0x0042bed0). (Porting pass 2, batch 10.)
#include "world_renderer.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "world_obj.h"
#include "art_row_blits_ports.h"
#include "dirty_rects_ports.h"

#define g_cellOriginX GLOBAL_AS(int, g_cellGrid_storage)            // cell table origin added to the cell's x
#define g_cellTable GLOBAL_PTR(uint32_t, g_visibleCells)         // 12-bit cell ids, (y << 6) | x
#define g_worldBuckets GLOBAL_PTR(CWorldObj *, g_tileBuckets)   // 64 x 64 bucket heads

// 0x004767c0: ECX = destination pixels, EDX = row width (stored in the blitter globals).
#define Blit_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x0044f8b0: invalidate a screen rect (ECX = &{x0, y0, x1, y1}).
#define Screen_Invalidate(o) PORT_THISCALL0(&DirtyRects_Invalidate, void, (o))()
// 0x0044fdf0: invalidate the whole screen; ECX is not read.
#define Screen_InvalidateAll() (InvalidateAll())

// CObj fields reached from CWorldObj: the graphic word (+0x38) and the signed tiledata offset byte (+0x3a).
static inline unsigned raw_graphic(CWorldObj *o)
{
    return RAW_AT(uint16_t, o, 0x38);
}
static inline int raw_flagByte(CWorldObj *o)
{
    return RAW_AT(int8_t, o, 0x3a);
}

struct RectI4 {
    int x0, y0, x1, y1;
};

// 0x0042a350  size 0x13a
// Builds the diagonal scan order of the 64 x 64 cells in two stack arrays, inverts it, and appends
// the cells with |x - y| < 0x18 and 0x18 < x + y < 0x68 to the visible-cell table.
// The stack probe call (0x0048b240, _chkstk) of the original is the compiler's own here.
void __cdecl WorldBuildVisibleCellTable()
{
    int16_t order[0x1000];
    int16_t inv[0x1000];

    {
        int acc = 0;
        for (int i = 0; i < 0x40; i++) {
            order[i] = static_cast<int16_t>(acc);
            order[0xfff - i] = static_cast<int16_t>(0xfff - acc);
            acc = acc + i + 2;
        }
    }
    {
        int row = 1;
        for (int base = 0x40; base < 0xfc0; base += 0x40) {
            for (int col = 0; col < 0x40; col++) {
                if (col + row < 0x40)
                    order[base + col] = static_cast<int16_t>(order[base + col - 0x40] + col + row);
            }
            row++;
        }
    }
    {
        int row = 0x3e;
        for (int base = 0xf80; base >= 0x40; base -= 0x40) {
            for (int col = 0x3f; col >= 0; col--) {
                if (col + row >= 0x40)
                    order[base + col] = static_cast<int16_t>(order[base + col + 0x40] + col + row - 0x7e);
            }
            row--;
        }
    }
    for (int i = 0; i < 0x1000; i++)
        inv[order[i]] = static_cast<int16_t>(i);

    uint32_t *out = g_cellTable + g_visibleCellCount;
    for (int n = 0; n < 0x1000; n++) {
        int cell = inv[n];
        int x = cell & 0x3f;
        int y = cell >> 6;
        int diff = x - y;
        int sum = x + y;
        if (diff > -0x18 && diff < 0x18 && sum > 0x18 && sum < 0x68) {
            *out = static_cast<uint32_t>(cell);
            g_visibleCellCount = g_visibleCellCount + 1;
            out++;
        }
    }
}
HOOK_FUNC(0x0042a350, WorldBuildVisibleCellTable);

// 0x0042a490  size 0x1d
CWorldRenderer *__thiscall CWorldRenderer::Reset()
{
    unk_1c = 0;
    unk_18 = 0;
    clearFirst = 0;
    memset(GLOBAL_PTR(uint32_t, g_missileTable), 0, 0x14 * 4);
    return this;
}
HOOK_METHOD(0x0042a490, CWorldRenderer, Reset);

// 0x0042a4b0  size 0x1b4
void __thiscall CWorldRenderer::RedrawRect(int *rc)
{
    Blit_SetTarget(g_backBuffer, 0x280);
    int topW = g_scrollY + rc[1];
    int bottomW = g_scrollY + rc[3];
    int leftW = g_scrollX + rc[0];
    int rightW = g_scrollX + rc[2];

    if (clearFirst != 0) {
        uint16_t *row = g_backBuffer + (rc[1] * 640 + rc[0]);
        for (int yy = rc[1]; yy < rc[3]; yy++) {
            memset(row, 0, static_cast<size_t>((rc[2] - rc[0]) * 2));
            row += 640;
        }
    }

    for (int i = 0; i < g_visibleCellCount; i++) {
        int cell = static_cast<int>(g_cellTable[i]);
        int bx = (cell + g_cellOriginX) & 0x3f;
        int by = ((cell >> 6) + g_cellOriginY) & 0x3f;
        CWorldObj *obj = g_worldBuckets[(by << 6) + bx];
        for (; obj != nullptr; obj = obj->next) {
            if (bottomW < obj->rectY0)
                continue;
            if (topW > obj->rectY1)
                continue;
            if (rightW < obj->rectX0)
                continue;
            if (leftW > obj->rectX1)
                continue;
            if (PORT_VSLOT0(obj, 0x3c, int)() == 0) {
                PORT_VSLOT1(obj, 0x08, void, int *)(rc);
            } else if (PORT_VSLOT0(obj, 0x18, int)() != 0) {
                unsigned graphic = raw_graphic(obj);
                if ((*reinterpret_cast<uint32_t *>(g_tileData + graphic * 0x28) & 0x20000) != 0) {
                    int idx = static_cast<int>(raw_flagByte(obj)) + static_cast<int>(graphic);
                    uint32_t saved = *reinterpret_cast<uint32_t *>(g_tileData + idx * 0x28);
                    *reinterpret_cast<uint32_t *>(g_tileData + idx * 0x28) = saved | 8;      // draw as if flag 8 were set
                    PORT_VSLOT1(obj, 0x08, void, int *)(rc);
                    idx = static_cast<int>(raw_flagByte(obj)) + static_cast<int>(raw_graphic(obj));
                    *reinterpret_cast<uint32_t *>(g_tileData + idx * 0x28) = saved;
                }
            }
        }
    }
}
HOOK_METHOD(0x0042a4b0, CWorldRenderer, RedrawRect);

// 0x0042bea0  size 0x30
int __fastcall WorldRectsOverlap(const int *a, const int *b)
{
    if (a[2] > b[0] && a[0] < b[2] && a[3] > b[1] && a[1] < b[3])
        return 1;
    return 0;
}
HOOK_FUNC(0x0042bea0, WorldRectsOverlap);

// 0x0042bd90  size 0x10e
// Marks the cell (x, y) visited and looks in it for a roof item (tiledata flag 0x10000000, slot 0x18 true,
// slot 0x24 false) within 6 of z. On the first such item the running minimum z is lowered and the four
// neighbouring cells are searched with that item's z. Returns the minimum z.
int __fastcall WorldRoofFloodFill(int minZ, int x, int y, int z)
{
    int idx = (x & 0x3f) + ((y & 0x3f) << 6);
    int hit = 0;                     // [ebp-8]
    CWorldObj *found = nullptr;      // edi
    if (g_roofVisited[idx] != 0)
        return minZ;
    CWorldObj *obj = g_worldBuckets[idx];
    g_roofVisited[idx] = 1;
    for (; obj != nullptr; obj = obj->next) {
        if (PORT_VSLOT0(obj, 0x18, int)() == 0)
            continue;
        found = obj;
        if (PORT_VSLOT0(obj, 0x24, int)() != 0)
            continue;
        if ((*reinterpret_cast<uint32_t *>(g_tileData + (raw_flagByte(obj) + raw_graphic(obj)) * 0x28) & 0x10000000) == 0)
            continue;
        if (abs(z - static_cast<int>(obj->z)) > 6)
            continue;
        hit = 1;
        if (obj->z < minZ)
            minZ = obj->z;
        break;
    }
    if (hit != 0) {
        minZ = WorldRoofFloodFill(minZ, x - 1, y, found->z);
        minZ = WorldRoofFloodFill(minZ, x + 1, y, found->z);
        minZ = WorldRoofFloodFill(minZ, x, y - 1, found->z);
        minZ = WorldRoofFloodFill(minZ, x, y + 1, found->z);
    }
    return minZ;
}
HOOK_FUNC(0x0042bd90, WorldRoofFloodFill);

// 0x0042bed0  size 0x1ee
// Recomputes the roof limits (+0x08, +0x0c) from the objects above the player in his cell and in the
// cell (x + 1, y + 1), flood-filling the roof cells, then invalidates the foliage whose hidden state changed.
void __thiscall CWorldRenderer::UpdateRoofHideLimit()
{
    int old8 = roofLimit;
    int oldC = roofLimit2;
    roofLimit2 = 0x7f;
    roofLimit = 0x7f;
    CWorldObj *player = reinterpret_cast<CWorldObj *>(g_player);
    if (player->inWorld == 0)
        return;
    int pz = player->z;
    int px = player->x;
    int py = player->y;

    CWorldObj *obj = g_worldBuckets[(px & 0x3f) + ((py & 0x3f) << 6)];
    for (; obj != nullptr; obj = obj->next) {
        if (PORT_VSLOT0(obj, 0x1c, int)() != 0)
            continue;
        int oz = obj->z;
        if (oz >= roofLimit)
            continue;
        if (oz <= pz + 0xe)
            continue;
        if (PORT_VSLOT0(obj, 0x24, int)() != 0)
            continue;
        unsigned fl = PORT_VSLOT0(obj, 0x40, unsigned)();
        if ((fl & 0x20004) != 0)
            continue;
        if ((fl & 0x10000000) != 0 && (fl & 0x200) == 0)
            continue;
        roofLimit = obj->z;
    }

    roofLimit2 = roofLimit;
    obj = g_worldBuckets[(((py + 1) & 0x3f) << 6) + ((px + 1) & 0x3f)];
    for (; obj != nullptr; obj = obj->next) {
        if (PORT_VSLOT0(obj, 0x1c, int)() != 0)
            continue;
        int oz = obj->z;
        if (oz >= roofLimit)
            continue;
        if (oz <= pz + 0xe)
            continue;
        if (PORT_VSLOT0(obj, 0x24, int)() != 0)
            continue;
        unsigned fl = PORT_VSLOT0(obj, 0x40, unsigned)();
        if ((fl & 0x204) != 0)
            continue;
        if ((fl & 0x10000000) == 0)
            continue;
        roofLimit2 = roofLimit = obj->z;
        memset(g_roofVisited, 0, 0x1000);
        roofLimit2 = WorldRoofFloodFill(roofLimit, obj->x, obj->y, obj->z);
    }

    CWorldObj *e = GLOBAL_AS(CWorldObj *, g_list20000Head);
    g_fadingPass = 1;
    for (; e != nullptr; e = RAW_AT(CWorldObj *, e, 0x58)) {
        if (e->inWorld == 0)
            continue;
        int cached = RAW_AT(int32_t, e, 0x70);
        int now = PORT_VSLOT0(e, 0x3c, int)();
        if (cached != now && e->inWorld != 0) {
            RectI4 r;
            r.y0 = e->rectY0 - g_scrollY;
            r.y1 = e->rectY1 - g_scrollY;
            r.x0 = e->rectX0 - g_scrollX;
            r.x1 = e->rectX1 - g_scrollX;
            Screen_Invalidate(&r);
        }
    }
    g_fadingPass = 0;

    roofLimit = roofLimit2;
    int floorZ = reinterpret_cast<CWorldObj *>(g_player)->z + 0x10;
    if (roofLimit < floorZ)
        roofLimit = floorZ;
    if (old8 != roofLimit || oldC != roofLimit2)
        Screen_InvalidateAll();
}
HOOK_METHOD(0x0042bed0, CWorldRenderer, UpdateRoofHideLimit);
