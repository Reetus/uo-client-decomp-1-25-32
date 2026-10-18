// Light tables and light-footprint drawing (second pass, batch 19): the circle tables the light bitmaps are
// clipped with, the shade / colour tables for the pixel format, the screen rectangle of a lit object and
// the two routines that copy a light footprint into the back buffer.
// Companion of light.cpp (0x0044f2e0 loader, 0x0044fe60 light pass).
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "raw_field.h"
#include "vcall.h"
#include "art_update_ops_ports.h"
#include "art_row_blits_hue_ports.h"
#include "dirty_rects_ports.h"

// Circle tables, 44 entries each (one per footprint row).

// Colour tables: 32 x 32 entries per channel.
#define lt_blue GLOBAL_PTR(int32_t, g_shadeTableB)
#define lt_red GLOBAL_PTR(int32_t, g_shadeTableR)
#define lt_green GLOBAL_PTR(int32_t, g_shadeTableG)
#define lt_lookupEnd GEND(lt_lookup)


// 0x00436c40: tile art record for a key (RET 4; ECX = 0x0068dd98 is not read by the port).
#define lt_TileArt(a1) ((uint16_t *)TileArtOrDefault(a1))
// 0x004761e0: copy `count` words, (ECX = dest, EDX = source, stack = count), RET 4.
#define lt_CopyWords(a1, a2, a3) (RowBlitCopyAll((uint16_t *)(a1), (uint16_t *)(a2), a3))
// 0x0044f8b0: invalidate the screen rectangle {left, top, right, bottom} in ECX.
#define lt_InvalidateRect(a1) (DirtyRects_Invalidate(a1))

// 0x0044ef30  size 0x120
// Build the circle tables of the 44-row light footprint. ECX is preserved and returned (the original is a
// thiscall that never uses it).
void *__fastcall BuildLightCircleTables(void *self)
{
    int i;
    int w;
    int v;
    int k;
    int t;
    int u;
    int z;
    int kb;
    int x;
    int y;

    lt_srcOffset[0] = 0;
    lt_rowStart[0] = 0x15;
    lt_rowLength[0] = 2;
    i = 1;
    w = 0x14;
    v = 0x58;
    k = 0;
    do {
        t = i * 2 + 2;
        if (v <= 0x2e)
            t = v - 4;
        lt_rowLength[k + 1] = t;
        u = i * 2;
        if (v < 0x2e)
            u = v;
        lt_srcOffset[k + 1] = lt_srcOffset[k] + u;
        z = w;
        if (v <= 0x2e)
            z = i - 0x15;
        lt_rowStart[k + 1] = z;
        i++;
        v -= 2;
        w--;
        k++;
    } while (v > 2);
    for (k = 22; k < 44; k++)
        lt_srcOffset[k] = lt_srcOffset[k] + 1;
    for (k = 0; k < 0x200; k++)
        reinterpret_cast<uint32_t *>(&lt_spanMaskA[0])[k] = 0;
    for (kb = 0; kb < 0xb0; kb += 4) {
        k = kb / 4;
        if (kb < 0x58) {
            x = lt_srcOffset[k];
            y = lt_rowLength[k] + x;
            lt_spanMaskA[x] = 0x7fff;
            lt_spanMaskB[y] = 0x7fff;
        }
        x = lt_srcOffset[k];
        y = lt_rowLength[k];
        lt_edgeA[x] = 0x1f;
        lt_edgeB[x + y] = 0x1f;
        lt_rowEnd[k] = lt_rowStart[k] + y;
    }
    return self;
}
HOOK_FUNC(0x0044ef30, BuildLightCircleTables);

// 0x0044f050  size 0xe1
// Build the shade table (level x channel value), the per-channel colour tables for the pixel format
// (shifts at 0x004afb48 / 0x004afb4c) and the 0x8000-entry lookup combining them. ECX preserved.
void *__fastcall BuildColourTables(void *self)
{
    int row = 0;
    int step = 0;
    int acc;
    int o;
    int cnt;
    int v;
    uint8_t *pb;
    int idx;
    uint32_t *out;
    int r;
    int g;

    do {
        acc = row << 5;
        pb = &g_lightLut[row];
        o = row << 5;
        cnt = 0x20;
        do {
            v = (acc + ((acc >> 31) & 0x1f)) >> 5;
            *pb = static_cast<uint8_t>(v);
            lt_blue[o] = v;
            lt_red[o] = v << (g_redShift16 & 0x1f);
            lt_green[o] = v << (g_greenShift16 & 0x1f);
            acc += step;
            pb += 0x20;
            o++;
            cnt--;
        } while (cnt != 0);
        row++;
        step--;
    } while (step > -0x20);
    idx = 0;
    out = &lt_lookup[0];
    do {
        r = idx >> (g_redShift16 & 0x1f);
        g = idx >> (g_greenShift16 & 0x1f);
        out++;
        out[-1] = static_cast<uint32_t>(lt_red4[r << 5]) | static_cast<uint32_t>(lt_green4[(g & 0x1f) << 5]) | static_cast<uint32_t>(lt_blue4[(idx & 0x1f) << 5]);
        idx++;
    } while (reinterpret_cast<uintptr_t>(out) < lt_lookupEnd);
    return self;
}
HOOK_FUNC(0x0044f050, BuildColourTables);

// 0x0044f140  size 0x199
// Invalidate the screen rectangle covered by this object's light footprint (ECX = object). The light
// level is the object's facing byte (+0x6e), or, when the object is carried by another, the light value
// of its tile data record; levels of 100 and over (no light) do nothing. The footprint size comes from
// lightidx.mul record `level`: width at +8, height at +0xa.
void __fastcall InvalidateLightRect(CObj *self)
{
    CObj *base;
    int level;
    uint8_t *rec;
    int width;
    int height;
    int x;
    int y;
    int rect[4];

    base = self;
    if (decomp::vcall(PORT_RET(int), self, 0x2c) != 0 && RAWF(CObj *, self, 0x7c) != nullptr)
        base = RAWF(CObj *, self, 0x7c);
    if (self == base)
        level = self->facing;
    else
        level = *reinterpret_cast<uint16_t *>(g_tileData + self->graphic * 0x28 + 0x10);
    level = level & 0xffff;
    if (level >= 0x64)
        return;
    rec = g_lightIndex + level * 12;
    width = *reinterpret_cast<uint16_t *>(rec + 8);
    height = *reinterpret_cast<uint16_t *>(rec + 0x0a);
    if (decomp::vcall(PORT_RET(int), base, 0x24) != 0) {
        if (base == g_player) {
            int px = base->x;
            int py = base->y;
            y = (px + py) * 22 + (5 - RAWF(int32_t, base, 0x190)) * 4 - g_scrollY + g_viewOffsetY;
            x = (px + g_scrollBase - py) * 22 - g_scrollX + g_viewOffsetX + 1;
        } else {
            int bx = base->x;
            int by = base->y;
            y = (bx + by) * 22 - RAWF(int32_t, base, 0x18c) + (5 - RAWF(int32_t, base, 0x190)) * 4 - g_scrollY;
            x = (bx + g_scrollBase - by) * 22 - RAWF(int32_t, base, 0x188) - g_scrollX + 1;
        }
    } else {
        y = base->sumXY * 22 - base->z * 4 - g_scrollY + 0x2b;
        x = base->key28 * 22 - g_scrollX;
    }
    rect[0] = x - (width >> 1);
    rect[2] = rect[0] + width;
    rect[1] = y - (height >> 1) - 0x16;
    rect[3] = rect[1] + height;
    lt_InvalidateRect(rect);
}
HOOK_FUNC(0x0044f140, InvalidateLightRect);

// 0x0044f3c0  size 0x144
// Copy one light footprint (tile art `id` in ECX, screen position x in EDX, y and the clip rectangle
// {left, top, right, bottom} on the stack, RET 8) row by row into the back buffer, clipped, with
// 0x004761e0. The footprint is 44 rows of at most 44 pixels; row k starts at column Z[k], ends at W[k],
// and its pixels begin A[k] words into the bitmap.
void __fastcall DrawLightFootprint(unsigned id, int x, int y, int *clip)
{
    uint16_t *art;
    int x0;
    int x1;
    int r0;
    int r1;
    int k;
    int count;
    int width;
    int n;
    int z;
    int w;
    uint8_t *dstRow;
    uint16_t *src;
    uint16_t *dst;

    art = lt_TileArt(id);
    if (art == nullptr)
        return;
    x0 = (clip[0] > x) ? clip[0] - x : 0;
    x1 = (clip[2] < x + 0x2c) ? clip[2] - x : 0x2c;
    if (x0 >= x1)
        return;
    r0 = (clip[1] > y) ? clip[1] - y : 0;
    r1 = (clip[3] < y + 0x2c) ? clip[3] - y : 0x2b;
    dstRow = reinterpret_cast<uint8_t *>(g_backBuffer) + ((y + r0) * 640 + x) * 2;
    width = x1 - x0;
    if (r0 >= r1)
        return;
    k = r0;
    count = r1 - r0;
    do {
        src = art + lt_srcOffset[k];
        z = lt_rowStart[k];
        w = lt_rowEnd[k];
        if (x0 > z) {
            dst = reinterpret_cast<uint16_t *>(dstRow) + x0;
            src = src + (x0 - z);
            if (w > x1) {
                n = width;
            } else {
                n = w - x0;
                if (n <= 0)
                    goto next;
            }
        } else {
            dst = reinterpret_cast<uint16_t *>(dstRow) + z;
            if (w > x1) {
                n = x1 - z;
                if (n <= 0)
                    goto next;
            } else {
                n = lt_rowLength[k];
            }
        }
        lt_CopyWords(dst, src, n);
    next:
        dstRow += 0x500;
        k++;
        count--;
    } while (count != 0);
}
HOOK_FUNC(0x0044f3c0, DrawLightFootprint);

// 0x0044f510  size 0x1c4
// The same footprint walk as 0x0044f3c0, but every pixel goes through the 16-bit colour remap table at
// 0x0068dd9c on its way into the back buffer. (ECX = art id, EDX = x, stack = y, clip; RET 8.)
void __fastcall DrawLightFootprintRemap(unsigned id, int x, int y, int *clip)
{
    uint16_t *art;
    int x0;
    int x1;
    int r0;
    int r1;
    int k;
    int count;
    int width;
    int n;
    int z;
    int w;
    uint8_t *dstRow;
    const uint16_t *src;
    uint16_t *dst;

    art = lt_TileArt(id);
    if (art == nullptr)
        return;
    x0 = (clip[0] > x) ? clip[0] - x : 0;
    x1 = (clip[2] < x + 0x2c) ? clip[2] - x : 0x2c;
    if (x0 >= x1)
        return;
    r0 = (clip[1] > y) ? clip[1] - y : 0;
    r1 = (clip[3] < y + 0x2c) ? clip[3] - y : 0x2b;
    width = x1 - x0;
    dstRow = reinterpret_cast<uint8_t *>(g_backBuffer) + ((y + r0) * 640 + x) * 2;
    if (r0 >= r1)
        return;
    k = r0;
    count = r1 - r0;
    do {
        src = art + lt_srcOffset[k];
        z = lt_rowStart[k];
        w = lt_rowEnd[k];
        if (x0 > z) {
            dst = reinterpret_cast<uint16_t *>(dstRow) + x0;
            src = src + (x0 - z);
            if (w > x1) {
                n = width;
                if (n == 0)
                    goto next;
            } else {
                n = w - x0;
                if (n <= 0)
                    goto next;
            }
        } else {
            dst = reinterpret_cast<uint16_t *>(dstRow) + z;
            if (w > x1) {
                n = x1 - z;
                if (n <= 0)
                    goto next;
            } else {
                n = lt_rowLength[k];
                if (n == 0)
                    goto next;
            }
        }
        do {
            *dst = g_colourMap[*src];
            src++;
            dst++;
            n--;
        } while (n != 0);
    next:
        dstRow += 0x500;
        k++;
        count--;
    } while (count != 0);
}
HOOK_FUNC(0x0044f510, DrawLightFootprintRemap);
