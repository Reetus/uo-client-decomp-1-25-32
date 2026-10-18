// CTerrainTile drawing and rectangle (0x0042ee90 .. 0x0042f6e0), and the position setter 0x0042eec0.
//
// __thiscall methods with stack arguments are written as __fastcall(this, <unused EDX>, args...), the same
// ECX + callee-cleaned stack layout.
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"
#include "pos3.h"
#include "raster.h"
#include "raw_field.h"
#include "terrain_tile.h"
#include "terrain_tile_draw_ports.h"
#include "light_tables_ports.h"
#include "file_manager.h"
#include "raster_pal_ports.h"
#include "obj.h"

#define g_cutawayOn GLOBAL_AS(int, g_worldPicker_storage)

// 0x0042eec0: store three shorts at ECX (x, y, z); stack: x | y << 16, z (RET 8).
// 0x0044f510 / 0x0044f3c0: ECX = tile id, EDX = x on screen, stack (y on screen, arg) (RET 8).
#define Tile_DrawSurfacePal(a1, a2, a3, a4) (DrawLightFootprintRemap((unsigned)(a1), a2, a3, (int *)(a4)))
#define Tile_DrawSurfaceFlat(a1, a2, a3, a4) (DrawLightFootprint((unsigned)(a1), a2, a3, (int *)(a4)))
// 0x004405d0 on the file manager at 0x0068d980: record of file pair (9, 10) (land textures).
#define Tile_LoadTexture(id, plen, pshift) \
    PORT_THISCALL5(&CFileManager::GetRecord, uint8_t *, GLOBAL_PTR(void, g_fileManager_storage), int, int, int, int *, int *)(9, 0xa, (id), (plen), (pshift))
// 0x00450c60: palette-mapped variant of the lit textured triangle, same arguments as 0x004503f0.
#define Tile_TrianglePal(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) (DrawTexturedTrianglePal(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10))

// One triangle of the tile through the rasterizer selected by `usePal`:
// DrawLitTexturedTriangle is the port of 0x004503f0, Tile_TrianglePal is 0x00450c60.
#define TILE_TRI(a, b, c)                                                                                             \
    do {                                                                                                              \
        if (usePal == 0)                                                                                              \
            DrawLitTexturedTriangle(dest, reinterpret_cast<const uint16_t *>(tex), sizeShift,                         \
                                    reinterpret_cast<const int *>(&v[tab[a]]), reinterpret_cast<const int *>(&v[tab[b]]), \
                                    reinterpret_cast<const int *>(&v[tab[c]]), self->lightLevel[tab[a]],             \
                                    self->lightLevel[tab[b]], self->lightLevel[tab[c]], clip); /* 0x004503f0 */       \
        else                                                                                                          \
            Tile_TrianglePal(dest, reinterpret_cast<const uint16_t *>(tex), sizeShift,                                \
                             reinterpret_cast<const int *>(&v[tab[a]]), reinterpret_cast<const int *>(&v[tab[b]]),   \
                             reinterpret_cast<const int *>(&v[tab[c]]), self->lightLevel[tab[a]],                    \
                             self->lightLevel[tab[b]], self->lightLevel[tab[c]], clip); /* 0x00450c60 */              \
    } while (0)

// 0x0042ee90  size 0x2b
// Set the tile id and copy the 6-byte coordinates {x | y << 16, z} through 0x0042eec0 to +0x20.
// ECX = tile, stack: tile id, pointer to the coordinates (RET 8).
void __fastcall TerrainTile_SetTileAndPos(CTerrainTile *self, int, int tileId, TileCoords *coords)
{
    self->tileId = tileId;
    PORT_THISCALL1(&Pos_SetXYZ, void, &self->x, Pos3)(*reinterpret_cast<const Pos3 *>(coords));   // 0x0042eec0
}
HOOK_FUNC(0x0042ee90, TerrainTile_SetTileAndPos);

// 0x0042eec0  size 0x1e
// Store x, y, z as three shorts at ECX. Stack: x | y << 16, z (an 8-byte structure by value), RET 8.
void __fastcall Pos_SetXYZ(int16_t *self, int, Pos3 pos)
{
    self[0] = pos.x;
    self[1] = pos.y;
    self[2] = pos.z;
}
HOOK_FUNC(0x0042eec0, Pos_SetXYZ);

// 0x0042ef20  size 0x73
// Draw the tile surface at its screen position; stack: one argument passed on to the surface routine
// (RET 4). Tile id 2 is skipped unless 0x004ba304 is set. The surface routine is chosen by the player
// vtable slot +0x8c.
void __thiscall CObjVt49f730::Draw(int *clip)
{
    int arg = reinterpret_cast<int>(clip);
    CTerrainTile *const self = reinterpret_cast<CTerrainTile *>(this);
    if (self->tileId == 2 && g_showHidden == 0)
        return;
    void *player = g_player;
    if (PORT_VSLOT0(player, 0x8c, int)() != 0)
        Tile_DrawSurfacePal(self->tileId, self->rectX0 - g_scrollX, self->rectY0 - g_scrollY, arg);   // 0x0044f510
    else
        Tile_DrawSurfaceFlat(self->tileId, self->rectX0 - g_scrollX, self->rectY0 - g_scrollY, arg);  // 0x0044f3c0
}
HOOK_VMETHOD(0x0042ef20, CObjVt49f730, Draw);

// 0x0042f640  size 0xa0
// Screen rectangle from the four corner heights: x from the key at +0x28, y from the min / max of the
// corner screen y values (22 per row, minus 4 per height step). ECX = tile, plain ret.
void __thiscall CTerrainTileOps::ComputeScreenRect()
{
    CTerrainTile *const self = this;
    int s = self->sumXY;
    int left = self->key28 * 22 - 0x16;
    int c0 = s * 22 - self->height[0] * 4;
    int c1 = (s + 1) * 22 - self->height[1] * 4;
    int mx = c0;
    int mn = c0;
    int c2;
    int c3;
    self->rectX0 = left;
    self->rectX1 = left + 0x2c;
    if (c1 > mx)
        mx = c1;
    if (c1 < mn)
        mn = c1;
    c2 = (s + 2) * 22 - self->height[2] * 4;
    if (c2 > mx)
        mx = c2;
    if (c2 < mn)
        mn = c2;
    c3 = (s + 1) * 22 - self->height[3] * 4;
    if (c3 > mx)
        mx = c3;
    if (c3 < mn)
        mn = c3;
    self->rectY1 = mx;
    self->rectY0 = mn;
}
HOOK_VMETHOD(0x0042f640, CTerrainTileOps, ComputeScreenRect);

// 0x0042f6e0  size 0x376
// Draw the land tile as two textured, lit triangles. ECX = tile, stack: clip rectangle (RET 4).
// Nothing is drawn when the tile is flagged flat (+0x70), for tile id 2 unless 0x004ba304 is set, or when
// the texture id is 0 / >= 0x1000. The triangle rasterizer is the plain one, or the palette variant
// when the player vtable slot +0x8c answers non-zero. With the cut-away flag 0x0069e0c0 set each
// triangle is only drawn when its diagonal corner height is at most 0x0069e0c4.
// ? the four vertex records are {x, y, tag} (tag 1..4); the original leaves the bytes after the tag
//   unwritten.
struct TileVertex {
    int x;
    int y;
    int tag;
};
void __thiscall CTerrainTileOps::Draw(int *clip)
{
    CTerrainTile *const self = this;
    TileVertex v[4];
    int len;
    int sizeShift;
    uint8_t *tex;
    int texId;
    int left;
    int rowY;
    int usePal;
    int *tab = self->triangle;
    int8_t *h = self->height;
    void *player;
    void *dest;

    if (self->flat != 0)
        return;
    if (self->tileId == 2 && g_showHidden == 0)
        return;
    texId = RAWF(uint16_t, g_landTable + self->tileId * 0x1c, 4);
    if (texId <= 0 || texId >= 0x1000)
        return;
    tex = Tile_LoadTexture(texId, &len, &sizeShift);   // 0x004405d0
    if (len == 0)
        return;
    left = self->rectX0 - g_scrollX;
    rowY = self->sumXY * 22 - g_scrollY;
    v[0].x = left + 0x16;
    v[0].y = rowY - h[0] * 4 - 1;
    v[0].tag = 1;
    v[1].x = left + 0x2e;
    v[1].y = rowY - h[1] * 4 + 0x16;
    v[1].tag = 2;
    v[2].x = left + 0x17;
    v[2].y = rowY - h[2] * 4 + 0x2d;
    v[2].tag = 3;
    v[3].x = left;
    v[3].y = rowY - h[3] * 4 + 0x16;
    v[3].tag = 4;
    player = g_player;
    usePal = (PORT_VSLOT0(player, 0x8c, int)() != 0) ? 1 : 0;
    dest = g_backBuffer;

    if (g_cutawayOn == 0) {
        if (h[0] == h[1] && h[1] == h[2] && h[3] - h[0] > 0x14) {
            TILE_TRI(0, 1, 2);   // 0x004503f0 or 0x00450c60
            v[0].x = v[0].x + 1;
            TILE_TRI(3, 4, 5);   // 0x004503f0 or 0x00450c60
            return;
        }
        TILE_TRI(0, 1, 2);   // 0x004503f0 or 0x00450c60
        TILE_TRI(3, 4, 5);   // 0x004503f0 or 0x00450c60
        return;
    }
    if (h[self->diag0] <= g_cutawayZ)
        TILE_TRI(0, 1, 2);   // 0x004503f0 or 0x00450c60
    if (h[self->diag1] > g_cutawayZ)
        return;
    TILE_TRI(3, 4, 5);   // 0x004503f0 or 0x00450c60
}
HOOK_VMETHOD(0x0042f6e0, CTerrainTileOps, Draw);
