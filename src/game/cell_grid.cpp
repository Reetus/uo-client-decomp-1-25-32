// CCellGrid (the 64 x 64 world-cell window) and the screen-point picker CWorldPicker.
// 0x0042fa60 .. 0x00430900. Cell lists are walked through the objects' original vtables
// (slots named in the comments are byte offsets).
#include "cell_grid.h"

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "raw_field.h"
#include "vcall.h"
#include "dirty_rects_ports.h"
#include "client_misc_ops_ports.h"
#include "circle_trans.h"
#include "mobile_cached_draw_ports.h"
#include "mobile_draw_ops_ports.h"
#include "file_manager.h"
#include "world_renderer.h"
#include "paperdoll_gump_ops_ports.h"
#include "art_record_ops_ports.h"
#include "map_world.h"

#define g_mapView GLOBAL_PTR(void, g_map_storage)
#define g_cellBuckets GLOBAL_PTR(CWorldObj *, g_tileBuckets)

// 0x0044fdf0: invalidate the whole screen.
#define InvalidateAll PORT_FUNC0(&InvalidateAll, void, __cdecl)
// 0x0044f8b0: ECX = RECT {left, top, right, bottom} in screen coordinates.
#define InvalidateRect_(o) PORT_THISCALL0(&DirtyRects_Invalidate, void, (o))()
// 0x00478cd0: option test (stub returning 0).
#define IsGameMaster() (Misc_Return0())
// 0x004244e0: circle-transparency test of an object (ECX = object).
#define CircleTransTest(self) (reinterpret_cast<CWorldObj_004244e0 *>(self)->OverlapsCircle())
// 0x00471040: ECX = mobile; nonzero when the mobile is drawn by the virtual slot 0x80 path.
#define MobileUsesCustomHit(o) PORT_THISCALL0(&Mobile_FindMountEntry, int, (o))()
// 0x0042cff0: ECX = body, EDX = facing, stack (&animIndex, &frame), RET 8.
#define GetStandAnimIndex(a1, a2, a3, a4) (Mobile_GetStandAnimIndex(a1, (uint32_t)(a2), a3, a4))
// 0x0042caf0: ECX = body, EDX = facing, stack (walkTotal, action, animFrame, phase, dirState,
// &animIndex, &frame, mobile), RET 0x20.
#define GetAnimFrame(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) (Mobile_GetAnimFrame((uint32_t)(a1), (uint32_t)(a2), a3, a4, a5, a6, a7, a8, a9, (CMobile *)(a10)))
// 0x004405d0: animation record (5, 6, animIndex, &a, &b) on the file manager at 0x0068d980.
#define AnimRecord(self, a1, a2, a3, a4, a5) (reinterpret_cast<CFileManager *>(self)->GetRecord(a1, a2, a3, a4, a5))
#define g_animFiles GLOBAL_PTR(void, g_fileManager_storage)
// 0x0042ac60: ECX = point x, EDX = point y, stack (mirror, sx, sy, sprite), RET 0x10.
#define HitTestRLESprite(a1, a2, a3, a4, a5, a6) (WorldHitTestRLESprite(a1, a2, a3, a4, a5, a6))
// 0x00466b60: ECX = layer, EDX = equipment array (mobile + 0xc4).
#define LayerIsCovered(a1, a2) (Paperdoll_LayerIsCovered(a1, (uint8_t *)(a2)))
// 0x00476160: ECX = art cache (0x0068d2d8), stack (art, x, y), RET 0xc; pixel (0 = transparent).
#define ArtPixel(o, a1, a2, a3) PORT_THISCALL3(&ArtRecord_PixelAt, uint16_t, (o), const uint8_t *, int, int)(a1, a2, a3)
#define g_artCache GLOBAL_PTR(void, g_artCache_storage)
// 0x0044f8b0 scroll-aware rect, 0x00446120 CMap::GetTileId (ECX = 0x0068dbb8, stack (x, y), RET 8).
#define Map_GetTileId(self, a1, a2) ((int)reinterpret_cast<CMap *>(self)->GetTileId(a1, a2))

// ---------------------------------------------------------------------------
// CCellGrid
// ---------------------------------------------------------------------------

// 0x0042fa60  size 0x23
// Clear the 0x1000 cells, cell origin (0, 0), window origin (-128, -128). Returns this.
CCellGrid *__thiscall CCellGrid::Ctor()
{
    memset(cells, 0, sizeof(cells));
    cellY = 0;
    cellX = 0;
    originY = -128;
    originX = -128;
    return this;
}
HOOK_METHOD(0x0042fa60, CCellGrid, Ctor);

// 0x0042fa90  size 0x22
// Purge every cell of the window, column-major (outer loop is the first PurgeCell argument).
void __thiscall CCellGrid::PurgeAll()
{
    for (int i = 0; i < 0x40; i++) {
        for (int j = 0; j < 0x40; j++)
            PurgeCell(i, j);   // 0x0042fac0
    }
}
HOOK_METHOD(0x0042fa90, CCellGrid, PurgeAll);

// 0x0042fac0  size 0xa3
// Walk the list of cell (x, y). An object whose virtual slot 0x2c or 0x1c returns nonzero is
// unlinked and its in-world flag cleared; the player is only invalidated (and re-added through
// slot 0x10 afterwards), every other object is released through slot 0x04.
void __thiscall CCellGrid::PurgeCell(int x, int y)
{
    int playerSeen = 0;
    CWorldObj **slot = &cells[y * 0x40 + x];
    CWorldObj *o = *slot;

    while (o != nullptr) {
        CWorldObj *nextObj = o->next;
        if (decomp::vcall(PORT_RET(int), o, 0x2c) != 0 || decomp::vcall(PORT_RET(int), o, 0x1c) != 0) {
            if (o->next != nullptr)
                o->next->prev = o->prev;
            if (o->prev != nullptr)
                o->prev->next = o->next;
            else
                *slot = o->next;
            o->inWorld = 0;
            if (o == reinterpret_cast<CWorldObj *>(g_player)) {
                InvalidateAll();
                playerSeen = 1;
            } else {
                decomp::vcall_void(o, 0x04);
            }
        }
        o = nextObj;
    }
    if (playerSeen != 0)
        decomp::vcall_void(g_player, 0x10);
}
HOOK_METHOD(0x0042fac0, CCellGrid, PurgeCell);

// 0x0042fb70  size 0x1d8
// Move the window to the view centre minus 0x20. The world is 0x1400 x 0x1000 tiles and wraps;
// a window origin near an edge is first shifted by a world width / height. Rows and columns
// that scrolled out are purged (the whole window when the jump is 0x40 or more).
void __thiscall CCellGrid::ScrollToView()
{
    int newX = g_viewCenterX - 0x20;
    int newY = g_viewCenterY - 0x20;
    int c, j;

    if (originX == newX && originY == newY)
        return;

    if (originX + 0x20 < 0x1400) {
        if (originX >= 0x13c0 && newX < 0)
            originX -= 0x1400;
        if (originX < 0 && newX > 0x13c0)
            originX += 0x1400;
        if (originY >= 0xfc0 && newY < 0)
            originY -= 0x1000;
        if (originY < 0 && newY > 0xfc0)
            originY += 0x1000;
    }

    int dx = newX - originX;
    int dy = newY - originY;
    if (abs(dx) < 0x40 && abs(dy) < 0x40) {
        if (dx > 0) {
            for (c = originX; c < newX; c++) {
                for (j = 0; j < 0x40; j++)
                    PurgeCell(c & 0x3f, j);   // 0x0042fac0
            }
        } else if (dx < 0) {
            for (c = newX; c < originX; c++) {
                for (j = 0; j < 0x40; j++)
                    PurgeCell(c & 0x3f, j);   // 0x0042fac0
            }
        }
        if (dy > 0) {
            for (c = originY; c < newY; c++) {
                for (j = 0; j < 0x40; j++)
                    PurgeCell(j, c & 0x3f);   // 0x0042fac0
            }
        } else if (dy < 0) {
            for (c = newY; c < originY; c++) {
                for (j = 0; j < 0x40; j++)
                    PurgeCell(j, c & 0x3f);   // 0x0042fac0
            }
        }
    } else {
        for (int a = 0; a < 0x40; a++) {
            for (j = 0; j < 0x40; j++)
                PurgeCell(a, j);   // 0x0042fac0
        }
    }

    originX = newX;
    originY = newY;
    cellX = newX & 0x3f;
    cellY = newY & 0x3f;
}
HOOK_METHOD(0x0042fb70, CCellGrid, ScrollToView);

// ---------------------------------------------------------------------------
// CWorldPicker
// ---------------------------------------------------------------------------

// 0x0042fd50  size 0x92c
// Pick the topmost object under screen point (x, y). The visible cells are walked from the last
// to the first, each cell list from its tail back (front-most first). An object qualifies when
// the point is inside its screen rect and it passes the virtual tests (0x3c, 0x38, 0x34 must be
// zero; slot 0x1c = terrain tile, 0x18 = pickable, 0x2c = mobile-like, 0x54 = corpse-like,
// 0x28 = the player, 0x24 = mobile, 0x74 / 0x78 = art, 0x80 / 0x84 = custom hit test / action),
// then a pixel test decides: the diamond of a land tile, an art pixel, or the animation frame
// (and the equipped layers) of a mobile. Returns the object or 0.
// ? The GetAnimFrame calls for the equipped layers of a non-player mobile pass the frame count of
// the body's record as the animFrame argument where the body's own call passes the mobile's
// +0x1aa; the assembly does so and it is kept.
CWorldObj *__thiscall CWorldPicker::GetObjectAt(int x, int y, int flag)
{
    int wx = x + g_scrollX;
    int wy = y + g_scrollY;
    int base, frame;        // [ebp-8], [ebp+8]
    int unusedA, unusedB;   // [ebp-0x14], [ebp-0x10]

    for (int i = g_visibleCellCount - 1; i >= 0; i--) {
        int cell = g_visibleCells[i];
        CWorldObj *o = g_cellBuckets[(((cell >> 6) + g_cellGrid.cellY) & 0x3f) * 0x40 + ((cell + g_cellGrid.cellX) & 0x3f)];
        if (o == nullptr)
            continue;
        while (o->next != nullptr)
            o = o->next;

        for (; o != nullptr; o = o->prev) {
            if (wx < o->rectX0 || wx >= o->rectX1)
                continue;
            if (wy < o->rectY0 || wy >= o->rectY1)
                continue;
            if (decomp::vcall(PORT_RET(int), o, 0x3c) != 0)
                continue;
            if (decomp::vcall(PORT_RET(int), o, 0x38) != 0)
                continue;
            if (decomp::vcall(PORT_RET(int), o, 0x34) != 0)
                continue;
            if (decomp::vcall(PORT_RET(int), o, 0x1c) != 0) {
                if (flag != 0)
                    continue;
                if (zFilter != 0 && o->z > maxZ)
                    continue;
            }

            if (decomp::vcall(PORT_RET(int), o, 0x1c) != 0) {
                // land tile: the 44 x 44 diamond
                int lx = static_cast<uint16_t>(wx - o->rectX0);
                int ly = static_cast<uint16_t>(wy - o->rectY0);
                if (lx + ly <= 0x14 || lx + ly >= 0x42)
                    continue;
                if (ly + (0x2b - lx) <= 0x14)
                    continue;
                if (ly + (0x2b - lx) < 0x42)
                    return o;
                continue;
            }

            if (decomp::vcall(PORT_RET(int), o, 0x18) == 0)
                continue;
            if (g_ccVal220 != 0) {
                if (CircleTransTest(o) != 0)
                    continue;
            }
            if (decomp::vcall(PORT_RET(int), o, 0x2c) != 0 && (RAWF(uint8_t, o, 0x98) & 0x80) != 0 && o != static_cast<CWorldObj *>(g_player) &&
                IsGameMaster() == 0 && g_player != nullptr && g_player->graphic != 0x3db)
                continue;

            if (decomp::vcall(PORT_RET(int), o, 0x54) != 0) {
                // corpse-like: the stand animation of the body stored at +0x3c
                CObj *obj = static_cast<CObj *>(o);
                if (obj->unk_3c >= 0x3e8)
                    continue;
                GetStandAnimIndex(obj->unk_3c, obj->facing, &base, &frame);
                uint8_t *anim = AnimRecord(g_animFiles, 5, 6, base, &unusedA, &unusedB);
                if (anim == nullptr)
                    continue;
                uint8_t *tbl = anim + 0x200;
                if (frame >= *reinterpret_cast<int *>(tbl))
                    continue;
                int off = reinterpret_cast<int *>(tbl)[frame + 1];
                int sx = (g_scrollBase + o->x - o->y) * 22 + 1;
                int sy = (o->y + o->x) * 22 + (5 - o->z) * 4;
                if (HitTestRLESprite(wx, wy, g_dirMirror[obj->facing & 0x7f], sx, sy, tbl + off + 8) != 0)
                    return o;
                continue;
            }

            if (decomp::vcall(PORT_RET(int), o, 0x28) != 0) {
                // the player
                CMobile *pl = static_cast<CMobile *>(g_player);
                if (pl->graphic >= 0x3e8)
                    continue;
                if (MobileUsesCustomHit(pl) != 0) {
                    if (decomp::vcall(PORT_RET(int), pl, 0x80, wx, wy) != 0)
                        return o;
                    continue;
                }
                int body = pl->graphic;
                if (body == 0x192)
                    body = 0x190;
                if (body == 0x193)
                    body = 0x191;
                int action = decomp::vcall(PORT_RET(int), pl, 0x84);
                GetAnimFrame(body, pl->facing, g_walkTick, action, pl->animFrame, pl->unk_1ac + pl->unk_1b8, g_walkDirState,
                             &base, &frame, pl);   // 0x0042caf0
                uint8_t *anim = AnimRecord(g_animFiles, 5, 6, base, &unusedA, &unusedB);
                if (anim == nullptr)
                    continue;
                uint8_t *tbl = anim + 0x200;
                if (frame >= *reinterpret_cast<int *>(tbl))
                    continue;
                int sx = g_viewOffsetX + (g_scrollBase - pl->y + pl->x) * 22 + 1;
                int sy = g_viewOffsetY + (pl->y + pl->x) * 22 + (5 - pl->viewZ) * 4;
                int off = reinterpret_cast<int *>(tbl)[frame + 1];
                if (HitTestRLESprite(wx, wy, g_dirMirror[pl->facing & 0x7f], sx, sy, tbl + off + 8) != 0)
                    return static_cast<CWorldObj *>(g_player);
                if (pl->graphic < 0x190)
                    continue;
                for (int k = 0; k < 0x19; k++) {
                    int layer = Mc_layerOrder[k + 25 * (pl->facing & 7)];
                    if (layer == 0x15)
                        continue;
                    if (pl->equip[layer] == nullptr)
                        continue;
                    if (LayerIsCovered(layer, &pl->equip[0]) != 0)   // 0x00466b60
                        continue;
                    int tileAnim = *reinterpret_cast<uint16_t *>(g_tileData + pl->equip[layer]->graphic * 0x28 + 0xc);
                    if (tileAnim < 0x190 || tileAnim >= 0x3e8)
                        continue;
                    GetAnimFrame(tileAnim, pl->facing, g_walkTick, action, pl->animFrame, pl->unk_1ac + pl->unk_1b8, g_walkDirState,
                                 &base, &frame, pl);   // 0x0042caf0
                    uint8_t *anim2 = AnimRecord(g_animFiles, 5, 6, base, &unusedA, &unusedB);
                    if (anim2 == nullptr)
                        continue;
                    uint8_t *tbl2 = anim2 + 0x200;
                    int off2 = reinterpret_cast<int *>(tbl2)[frame + 1];
                    if (off2 == 0)
                        continue;
                    if (HitTestRLESprite(wx, wy, g_dirMirror[pl->facing & 0x7f], sx, sy, tbl2 + off2 + 8) != 0)
                        return static_cast<CWorldObj *>(g_player);
                }
                continue;
            }

            if (decomp::vcall(PORT_RET(int), o, 0x24) != 0) {
                // another mobile
                CMobile *m = static_cast<CMobile *>(static_cast<CObj *>(o));
                if (m->graphic >= 0x3e8)
                    continue;
                if (MobileUsesCustomHit(m) != 0) {
                    if (decomp::vcall(PORT_RET(int), m, 0x80, wx, wy) != 0)
                        return o;
                    continue;
                }
                int body = m->graphic;
                if (body == 0x192)
                    body = 0x190;
                if (body == 0x193)
                    body = 0x191;
                int action = decomp::vcall(PORT_RET(int), m, 0x84);
                GetAnimFrame(body, m->facing, m->moveTick, action, m->animFrame, m->unk_1ac + m->unk_1b8, m->moving, &base, &frame,
                             m);   // 0x0042caf0
                uint8_t *anim = AnimRecord(g_animFiles, 5, 6, base, &unusedA, &unusedB);
                if (anim == nullptr)
                    continue;
                uint8_t *tbl = anim + 0x200;
                int count = *reinterpret_cast<int *>(tbl);
                if (frame >= count)
                    continue;
                int sx = (g_scrollBase + m->x - m->y) * 22 - m->unk_188 + 1;
                int sy = (m->y + m->x) * 22 + (5 - m->viewZ) * 4 - m->unk_18c;
                int off = reinterpret_cast<int *>(tbl)[frame + 1];
                if (HitTestRLESprite(wx, wy, g_dirMirror[m->facing & 0x7f], sx, sy, tbl + off + 8) != 0)
                    return o;
                if (body < 0x190)
                    continue;
                for (int k = 0; k < 0x19; k++) {
                    int layer = Mc_layerOrder[k + 25 * (m->facing & 7)];
                    if (layer == 0x15)
                        continue;
                    if (m->equip[layer] == nullptr)
                        continue;
                    if (LayerIsCovered(layer, &m->equip[0]) != 0)   // 0x00466b60
                        continue;
                    int tileAnim = *reinterpret_cast<uint16_t *>(g_tileData + m->equip[layer]->graphic * 0x28 + 0xc);
                    if (tileAnim < 0x190 || tileAnim >= 0x3e8)
                        continue;
                    GetAnimFrame(tileAnim, m->facing, m->moveTick, action, count, m->unk_1ac + m->unk_1b8, m->moving, &base, &frame,
                                 m);   // 0x0042caf0
                    uint8_t *anim2 = AnimRecord(g_animFiles, 5, 6, base, &unusedA, &unusedB);
                    if (anim2 == nullptr)
                        continue;
                    uint8_t *tbl2 = anim2 + 0x200;
                    int off2 = reinterpret_cast<int *>(tbl2)[frame + 1];
                    if (off2 == 0)
                        continue;
                    if (HitTestRLESprite(wx, wy, g_dirMirror[m->facing & 0x7f], sx, sy, tbl2 + off2 + 8) != 0)
                        return o;
                }
                continue;
            }

            // static item or multi part: art pixel test
            {
                CObj *obj = static_cast<CObj *>(o);
                int g = obj->graphic;
                if (g == 1 || (g > 0x2197 && g <= 0x21a2)) {
                    if (g_showHidden == 0)
                        continue;
                }
                uint8_t *art = decomp::vcall(PORT_RET(uint8_t *), o, 0x74);
                uint8_t *art2 = decomp::vcall(PORT_RET(uint8_t *), o, 0x78);
                int lx = static_cast<uint16_t>(wx - o->rectX0);
                int ly = static_cast<uint16_t>(wy - o->rectY0);
                if (art != nullptr) {
                    if (ArtPixel(g_artCache, art, lx, ly) != 0)   // 0x00476160
                        return o;
                    if (art2 == nullptr)
                        continue;
                    if (ArtPixel(g_artCache, art, lx - 5, ly - 5) != 0)   // 0x00476160
                        return o;
                    continue;
                }
                if (lx + ly <= 0x14 || lx + ly >= 0x42)
                    continue;
                if (ly + (0x2b - lx) <= 0x14)
                    continue;
                if (ly + (0x2b - lx) < 0x42)
                    return o;
            }
        }
    }
    return nullptr;
}
HOOK_METHOD(0x0042fd50, CWorldPicker, GetObjectAt);

// ---------------------------------------------------------------------------
// Cell-list helpers
// ---------------------------------------------------------------------------

// 0x00430680  size 0x101
// Among the land tiles (slot 0x1c) whose screen rect contains the point, the one whose rect
// centre is nearest by |dx| + |dy| (below 9999). Visible cells are walked last to first, each
// list from its head.
CWorldObj *__stdcall CellGrid_GetNearestTerrainAt(int x, int y)
{
    int wx = x + g_scrollX;
    int wy = y + g_scrollY;
    int best = 9999;
    CWorldObj *bestObj = nullptr;

    for (int i = g_visibleCellCount - 1; i >= 0; i--) {
        int cell = g_visibleCells[i];
        CWorldObj *o = g_cellBuckets[(((cell >> 6) + g_cellGrid.cellY) & 0x3f) * 0x40 + ((cell + g_cellGrid.cellX) & 0x3f)];
        while (o != nullptr) {
            if (decomp::vcall(PORT_RET(int), o, 0x1c) != 0 && wx >= o->rectX0 && wx < o->rectX1 && wy >= o->rectY0 && wy < o->rectY1) {
                int dx = wx - ((o->rectX0 + o->rectX1) >> 1);
                int dy = wy - ((o->rectY0 + o->rectY1) >> 1);
                int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                if (dist < best) {
                    bestObj = o;
                    best = dist;
                }
            }
            o = o->next;
        }
    }
    return bestObj;
}
HOOK_FUNC(0x00430680, CellGrid_GetNearestTerrainAt);

// 0x00430790  size 0x37
// Last object in the list of cell (x & 0x3f, y & 0x3f), or null for an empty cell. Returned in EAX.
CWorldObj *__stdcall CellGrid_WalkCell(int x, int y)
{
    CWorldObj *obj = g_cellBuckets[(y & 0x3f) * 0x40 + (x & 0x3f)];
    if (obj == nullptr)
        return nullptr;
    while (obj->next != nullptr)
        obj = obj->next;
    return obj;
}
HOOK_FUNC(0x00430790, CellGrid_WalkCell);

// 0x004307d0  size 0xab
// For every land tile (slot 0x1c) in the 4096 cells that is in the world and whose map tile id
// matches `tile` in the low 15 bits, invalidate its screen rect (scrolled to screen space).
void __stdcall CellGrid_InvalidateTerrainTile(uint32_t tile)
{
    for (int row = 0; row < 0x1000; row += 0x40) {
        for (int x = 0; x < 0x40; x++) {
            for (CWorldObj *o = g_cellBuckets[row + x]; o != nullptr; o = o->next) {
                if (decomp::vcall(PORT_RET(int), o, 0x1c) != 0) {
                    uint32_t id = static_cast<uint32_t>(Map_GetTileId(g_mapView, o->x, o->y));
                    if (((id ^ tile) & 0x7fff) == 0 && o->inWorld != 0) {
                        int rect[4];
                        rect[1] = o->rectY0 - g_scrollY;
                        rect[3] = o->rectY1 - g_scrollY;
                        rect[0] = o->rectX0 - g_scrollX;
                        rect[2] = o->rectX1 - g_scrollX;
                        InvalidateRect_(rect);   // 0x0044f8b0
                    }
                }
            }
        }
    }
}
HOOK_FUNC(0x004307d0, CellGrid_InvalidateTerrainTile);

// 0x00430880  size 0x72
// For every in-world pickable (slot 0x18) object that is not a mobile (slot 0x24) and whose
// graphic + signed byte at +0x3a equals `id`: slot 0x14, then slot 0x10 (unlink, relink).
void __stdcall CellGrid_RelinkById(int id)
{
    for (int row = 0; row < 0x1000; row += 0x40) {
        for (int x = 0; x < 0x40; x++) {
            for (CWorldObj *o = g_cellBuckets[row + x]; o != nullptr; o = o->next) {
                if (o->inWorld != 0 && decomp::vcall(PORT_RET(int), o, 0x18) != 0 && decomp::vcall(PORT_RET(int), o, 0x24) == 0) {
                    CObj *obj = static_cast<CObj *>(o);
                    if (obj->graphic + static_cast<int8_t>(obj->unk_3a) == id) {
                        decomp::vcall_void(o, 0x14);
                        decomp::vcall_void(o, 0x10);
                    }
                }
            }
        }
    }
}
HOOK_FUNC(0x00430880, CellGrid_RelinkById);
