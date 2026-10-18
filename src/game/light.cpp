#include "light.h"

#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "vcall.h"
#include "mapped_file.h"


// 0x0044f2e0  size 0xd1
// Load the light tables. Each file object is a 0x90-byte MappedFile (new MappedFile(full path), ctor 0x0047e810)
// whose view comes from 0x0047e8d0. Returns its incoming ECX.
void *__fastcall LoadLightTables(void *self)
{
    g_lightIdxFile = new MappedFile(g_app.ResolveDataPath(const_cast<char *>("lightidx.mul")));
    g_lightFile = new MappedFile(g_app.ResolveDataPath(const_cast<char *>("light.mul")));

    g_lightIndex = decomp::thiscall_at(PORT_RET(uint8_t *), PORT_ADDR(&MappedFile::OpenReadOnly), g_lightIdxFile);
    g_lightBitmaps = decomp::thiscall_at(PORT_RET(uint8_t *), PORT_ADDR(&MappedFile::OpenReadOnly), g_lightFile);
    return self;
}
HOOK_FUNC(0x0044f2e0, LoadLightTables);

// 0x0044fe60  size 0x586
// Walks the rect in 64x48 pixel tiles. For each tile: fill a 64x48 byte light buffer with the ambient
// level, subtract each visible light's footprint (index table g_lightIndex / bitmaps g_lightBitmaps) for
// every light object on the list at DAT_0068ce60, then remap the tile's 16bpp pixels in the 640-wide back
// buffer through the LUT at 0x6f92a8 (555 when DAT_0075826c == 0, 565 otherwise).
// Field meanings of the light objects are offsets only (see CObj / CMobile).
// ? the source row offset of the footprint is (clipTop - lightTop) * the +0xa field of the index record
//   while the per-row stride is the +8 field, as the assembly multiplies; kept as is.
void __thiscall LightPass::Apply()
{
    int y0 = top;
    if (y0 >= bottom)
        return;
    CObj *player = g_player;
    int rowOff = y0 * 640;
    do {
        int x0 = left;
        if (x0 < right) {
            int x0Next = x0 + 0x40;
            bool firstTile = true;
            do {
                if (!firstTile)
                    player = g_player;   // the first tile of a row uses the value loaded before the row
                firstTile = false;

                int xEnd = x0Next;
                if (right < x0Next)
                    xEnd = right;
                int yEnd = y0 + 0x30;
                if (bottom < yEnd)
                    yEnd = bottom;

                int level = g_lightAmbient - g_playerLight;
                if (level < 0) {
                    level = 0;
                } else if (player != nullptr) {
                    if (decomp::vcall(PORT_RET(int), player, 0x8c) != 0)
                        level = 0;
                }
                uint8_t buf[0xc00];   // 64 x 48 byte light buffer
                memset(buf, static_cast<uint8_t>(level), 0xc00);

                for (CObj *node = g_lightListHead; node != nullptr; node = node->link48) {
                    CObj *base = node;
                    if (decomp::vcall(PORT_RET(int), node, 0x2c) != 0) {
                        CObj *owner = static_cast<CDynamicObj *>(node)->container;
                        if (owner != nullptr)
                            base = owner;
                    }
                    if (decomp::vcall(PORT_RET(int), base, 0x3c) != 0)
                        continue;
                    if (base->inWorld == 0)
                        continue;

                    unsigned idx = (((static_cast<unsigned>(static_cast<uint16_t>(base->y)) + 1) & 0x3f) << 6) +
                                   ((static_cast<unsigned>(static_cast<uint16_t>(base->x)) + 1) & 0x3f);
                    CObj *other = g_tileBuckets[idx];
                    if (other != nullptr) {
                        do {
                            if (static_cast<int>(other->z) > static_cast<int>(base->z) + 4) {
                                if (decomp::vcall(PORT_RET(int), other, 0x24) == 0 && decomp::vcall(PORT_RET(int), other, 0x38) == 0 &&
                                    decomp::vcall(PORT_RET(int), other, 0x3c) == 0) {
                                    if (decomp::vcall(PORT_RET(int), other, 0x1c) != 0)
                                        break;
                                    if (!(decomp::vcall(PORT_RET(uint8_t), other, 0x40) & 4))
                                        break;
                                }
                            }
                            other = static_cast<CObj *>(other->next);
                        } while (other != nullptr);
                        if (other != nullptr)
                            continue;
                    }

                    int v;
                    if (node == base)
                        v = node->facing;
                    else
                        v = *reinterpret_cast<uint16_t *>(g_tileData + static_cast<unsigned>(node->graphic) * 0x28 + 0x10);
                    if (v >= 100)
                        continue;

                    int vOff = v * 12;
                    int bw = *reinterpret_cast<uint16_t *>(g_lightIndex + vOff + 8);
                    int bh = *reinterpret_cast<uint16_t *>(g_lightIndex + vOff + 0xa);
                    int px;
                    int py;
                    if (decomp::vcall(PORT_RET(int), base, 0x24) != 0) {
                        CMobile *pl = static_cast<CMobile *>(g_player);
                        if (base == pl) {
                            int lx = pl->x;
                            int ly = pl->y;
                            py = (ly + lx) * 11 * 2 + (5 - pl->viewZ) * 4 - g_scrollY + g_viewOffsetY;
                            px = (g_scrollBase - ly + lx) * 11 * 2 - g_scrollX + g_viewOffsetX + 1;
                        } else {
                            CMobile *bm = static_cast<CMobile *>(base);
                            int ly = bm->y;
                            int lx = bm->x;
                            py = (lx + ly) * 11 * 2 + (5 - bm->viewZ) * 4 - bm->unk_18c - g_scrollY;
                            px = (g_scrollBase - ly + lx) * 11 * 2 - bm->unk_188 - g_scrollX + 1;
                        }
                    } else {
                        py = base->sumXY * 11 * 2 - static_cast<int>(base->z) * 4 - g_scrollY + 0x2b;
                        px = base->key28 * 11 * 2 - g_scrollX;
                    }
                    int sx = px - (bw >> 1);
                    int sy = py - (bh >> 1) - 0x16;
                    int sxEnd = bw + sx;
                    int syEnd = bh + sy;
                    if (sxEnd <= x0)
                        continue;
                    if (syEnd <= y0)
                        continue;
                    if (sx >= xEnd)
                        continue;
                    if (sy >= yEnd)
                        continue;

                    int cx0 = x0;
                    if (sx >= x0)
                        cx0 = sx;
                    int cy0 = y0;
                    if (sy >= y0)
                        cy0 = sy;
                    int cx1 = sxEnd;
                    if (sxEnd >= xEnd)
                        cx1 = xEnd;
                    int cy1 = bh + sy;
                    if (cy1 >= yEnd)
                        cy1 = yEnd;

                    int d = cy0 - y0;
                    int tileW = xEnd - x0;
                    unsigned char *dstStart = buf + tileW * d;
                    // (cy0 - sy) * bh: the assembly multiplies by the +0xa field here (?), the row stride below is the +8 field
                    const signed char *srcRow = reinterpret_cast<const signed char *>(
                        g_lightBitmaps + (cy0 - sy) * bh + *reinterpret_cast<int32_t *>(g_lightIndex + vOff) - sx + x0);
                    int rows = cy1 - y0;
                    if (d >= rows)
                        continue;
                    int colStart = cx0 - x0;
                    int colEnd = cx1 - x0;
                    rows = rows - d;
                    do {
                        if (colStart < colEnd) {
                            int cnt = colEnd - colStart;
                            unsigned char *pDst = dstStart + colStart;
                            const signed char *pSrc = srcRow + colStart;
                            do {
                                int diff = static_cast<int>(static_cast<signed char>(*pDst)) - static_cast<int>(*pSrc);
                                if (diff < 0)
                                    diff = 0;
                                if (diff > 0x1f)
                                    diff = 0x1f;
                                *pDst = static_cast<unsigned char>(diff);
                                pDst++;
                                pSrc++;
                                cnt--;
                            } while (cnt != 0);
                        }
                        dstStart += xEnd - x0;
                        srcRow += bw;
                        rows--;
                    } while (rows != 0);
                }

                // remap the tile's pixels through the LUT
                uint16_t *pixRow = g_backBuffer + (x0 + rowOff);
                int w = xEnd - x0;
                const unsigned char *lightRow = buf;
                if (g_is565 == 0) {
                    if (y0 < yEnd) {
                        int rowsLeft = yEnd - y0;
                        do {
                            uint16_t *pix = pixRow;
                            const unsigned char *lp = lightRow;
                            if (w > 0) {
                                int cnt = w;
                                do {
                                    int dl = static_cast<signed char>(*lp);
                                    if (dl < 0x20) {
                                        const uint8_t *tbl = g_lightLut + dl * 32;
                                        unsigned px16 = *pix;
                                        int r = static_cast<int>(px16) >> 10;
                                        int g = (static_cast<int>(px16) >> 5) & 0x1f;
                                        int b = px16 & 0x1f;
                                        *pix = static_cast<uint16_t>((((tbl[r] << 5) | tbl[g]) << 5) | tbl[b]);
                                    }
                                    pix++;
                                    lp++;
                                    cnt--;
                                } while (cnt != 0);
                            }
                            pixRow += 640;
                            lightRow += w;
                            rowsLeft--;
                        } while (rowsLeft != 0);
                    }
                } else {
                    if (y0 < yEnd) {
                        int rowsLeft = yEnd - y0;
                        do {
                            uint16_t *pix = pixRow;
                            const unsigned char *lp = lightRow;
                            if (w > 0) {
                                int cnt = w;
                                do {
                                    int dl = static_cast<signed char>(*lp);
                                    if (dl < 0x20) {
                                        const uint8_t *tbl = g_lightLut + dl * 32;
                                        unsigned px16 = *pix;
                                        int r = static_cast<int>(px16) >> 10;
                                        int g = (static_cast<int>(px16) >> 5) & 0x1f;
                                        int b = px16 & 0x1f;
                                        *pix = static_cast<uint16_t>((((tbl[r] << 5) | tbl[g]) << 6) | tbl[b]);
                                    }
                                    pix++;
                                    lp++;
                                    cnt--;
                                } while (cnt != 0);
                            }
                            pixRow += 640;
                            lightRow += w;
                            rowsLeft--;
                        } while (rowsLeft != 0);
                    }
                }

                x0 += 0x40;
                x0Next += 0x40;
            } while (x0 < right);
            player = g_player;
        }
        y0 += 0x30;
        rowOff += 0x7800;   // 48 * 640
    } while (y0 < bottom);
}
HOOK_METHOD(0x0044fe60, LightPass, Apply);
