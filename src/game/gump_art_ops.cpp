// Gump art helpers next to the art drawers in gump_art.cpp: art size, pixel lookup and the
// clipped bitmap blitters used by the gump frame code. gumps.cxx area.
#include <windows.h>

#include <cstdint>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "file_manager.h"

// Art loader object at 0x0068d980; 0x004405d0 returns the row table (index per row,
// then {len << 16 | colour} runs) and fills in ok and dims (hi16 width, lo16 height).
#define GAO_LOADER GLOBAL_PTR(void, g_fileManager_storage)
#define GAO_ArtLoad(self, a1, a2, a3, a4, a5) ((int *)reinterpret_cast<CFileManager *>(self)->GetRecord(a1, a2, a3, a4, (int *)(a5)))

// 0x00433e70  size 0x55
// Width and height of gump art artId; both 0 when the art does not exist.
// ECX = art id, EDX = &w, stack = &h.
void __fastcall GumpArtSizeImpl(int artId, int *outW, int *outH)
{
    int ok;
    unsigned dims;

    if (GAO_ArtLoad(GAO_LOADER, 0xb, 0xc, artId, &ok, &dims) != nullptr) {
        *outW = static_cast<int>(dims) >> 16;
        *outH = static_cast<int>(dims & 0xffff);
        return;
    }
    *outW = 0;
    *outH = 0;
}
HOOK_FUNC(0x00433e70, GumpArtSizeImpl);

// 0x00434260  size 0x7d
// Blit a w x h bitmap of 16-bit pixels at (x, y) of dest (stride pixels wide, destH rows)
// row by row with rowBlit, clipped on the right and bottom only.
// ECX = row blitter, EDX = src; stack: w, h, x, y, dest, stride, destH.
void __fastcall GumpBlitBitmap(RowBlitFn rowBlit, uint16_t *src, int w, int h, int x, int y,
                                      uint16_t *dest, int stride, int destH)
{
    int cw = w;
    int rows;
    uint16_t *dst = dest + (stride * y + x);
    const int srcStride = w * 2;     // bytes, from the unclipped width
    const int dstStride = stride * 2;

    if (x + w > stride)
        cw = stride - x;
    if (h + y > destH)
        rows = destH - y;
    else
        rows = h;
    if (rows > 0) {
        do {
            rowBlit(dst, src, cw);
            src = reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(src) + srcStride);
            dst = reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(dst) + dstStride);
            rows--;
        } while (rows != 0);
    }
}
HOOK_FUNC(0x00434260, GumpBlitBitmap);

// 0x004342e0  size 0x123
// Like 0x00434260 but clipped on all four sides to clip = {left, top, right, bottom}.
// ECX = row blitter, EDX = src; stack: clip, w, h, x, y, dest, stride, destH.
void __fastcall GumpBlitBitmapClipped(RowBlitFn rowBlit, uint16_t *src, const RECT *clip, int w, int h,
                                             int x, int y, uint16_t *dest, int stride, int destH)
{
    int right = clip->right;
    int bottom, xEnd, yEnd, skipX, cw, rowStart, lastRow, count, rows;
    uint16_t *dst;
    uint16_t *srcp;

    if (x >= right)
        return;
    bottom = clip->bottom;
    if (y >= bottom)
        return;
    xEnd = x + w;
    if (xEnd < clip->left)
        return;
    yEnd = h + y;
    if (yEnd < clip->top)
        return;

    if (clip->left > x)
        skipX = clip->left - x;
    else
        skipX = 0;
    if (right < xEnd)
        cw = right - x;
    else
        cw = w;
    if (cw + x >= stride)
        cw = stride - x - 1;
    if (skipX >= cw)
        return;

    if (clip->top > y)
        rowStart = clip->top - y;
    else
        rowStart = 0;
    if (bottom < yEnd)
        lastRow = bottom - y - 1;
    else
        lastRow = h - 1;

    dst = dest + ((rowStart + y) * stride + skipX + x);
    count = cw - skipX;
    if (lastRow + y >= destH)
        lastRow = destH - y - 1;
    srcp = src + (rowStart * w + skipX);
    if (rowStart > lastRow)
        return;

    rows = lastRow - rowStart + 1;
    do {
        rowBlit(dst, srcp, count);
        srcp += w;
        dst += stride;
        rows--;
    } while (rows != 0);
}
HOOK_FUNC(0x004342e0, GumpBlitBitmapClipped);

// 0x00434410  size 0x95
// Colour of pixel (x, y) of gump art artId (0 when out of range or missing), converted to
// 565 when the display is 565. ECX = art id, EDX = x, stack = y.
uint16_t __fastcall GumpArtPixel(int artId, int x, int y)
{
    int ok;
    unsigned dims;
    int *rowTable;
    int w, h, acc, e;
    const int *run;
    unsigned colour;

    e = 0;
    rowTable = GAO_ArtLoad(GAO_LOADER, 0xb, 0xc, artId, &ok, &dims);
    if (ok == 0)
        return 0;
    h = static_cast<int>(dims & 0xffff);
    w = static_cast<int>(dims) >> 16;
    if (y >= h || x >= w)
        return 0;

    run = rowTable + rowTable[y];
    acc = 0;
    if (x > 0) {
        do {
            e = *run++;
            acc += e >> 16;
        } while (acc < x);
    }
    colour = static_cast<unsigned>(e) & 0xffff;
    if (g_is565 != 0)
        colour = (colour & 0x1f) | ((colour & 0x7fe0) << 1);
    return static_cast<uint16_t>(colour);
}
HOOK_FUNC(0x00434410, GumpArtPixel);
