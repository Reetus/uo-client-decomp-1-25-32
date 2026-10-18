// Gump art drawing: RLE art records from the art file pair 0xb/0xc expanded
// row by row and blitted into a 16-bit buffer. gumps.cxx area.
#include <windows.h>

#include <cstdint>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "file_manager.h"

// Art loader object at 0x0068d980; 0x004405d0 returns the row table (index per row,
// then {len << 16 | colour} runs) and fills in ok and dims (hi16 width, lo16 height).
#define g_artLoader GLOBAL_PTR(void, g_fileManager_storage)
#define Art_Load(self, a1, a2, a3, a4, a5) ((int *)reinterpret_cast<CFileManager *>(self)->GetRecord(a1, a2, a3, a4, (int *)(a5)))

// Expand one RLE row into line[] (colour runs of n pixels).
static void expand_row(int16_t *line, const int *rle, int w)
{
    int pos = 0;
    if (w > 0) {
        do {
            int e = *rle++;
            int16_t colour = static_cast<int16_t>(e);
            int n = e >> 16;
            if (n > 0) {
                int16_t *p = &line[pos];
                for (int k = n >> 1; k != 0; k--) {          // rep stosd
                    *reinterpret_cast<uint32_t *>(p) =
                        (static_cast<uint32_t>(static_cast<uint16_t>(colour)) << 16) | static_cast<uint16_t>(colour);
                    p += 2;
                }
                for (int k = n & 1; k != 0; k--)             // rep stosw
                    *p++ = colour;
                pos += n;
            }
        } while (pos < w);
    }
}

// 0x00433ed0  size 0x18a
// Draw gump art artId (RLE) into the 16-bit buffer dest, clipped to the buffer.
// ECX = row blitter, EDX = art id; stack: x, y, dest, stride, destH.
void __fastcall DrawGumpArt(RowBlitFn rowBlit, int artId, int x, int y, uint16_t *dest, int stride, int destH)
{
    int16_t line[1024];
    int ok;
    unsigned dims;
    int *rowTable;
    int w, lastRow, xskip, cw, yskip, rows;
    uint16_t *dst;
    int *rowIdx;

    rowTable = Art_Load(g_artLoader, 0xb, 0xc, artId, &ok, &dims);
    if (ok == 0)
        return;

    w = static_cast<int>(dims) >> 16;
    lastRow = static_cast<int>(dims & 0xffff) - 1;
    xskip = 0;
    cw = w;
    yskip = 0;
    if (x < 0) {
        xskip = -x;
        cw = w + x;
    }
    if (y < 0)
        yskip = -y;
    if (cw + x > stride)
        cw = stride - x;
    if (cw < 1)
        return;
    if (lastRow + y >= destH)
        lastRow = destH - y - 1;

    dst = dest + ((yskip + y) * stride + xskip + x);
    if (yskip > lastRow)
        return;

    rows = lastRow - yskip + 1;
    rowIdx = rowTable + yskip;
    do {
        expand_row(line, rowTable + *rowIdx, w);

        int16_t *src = &line[xskip];
        rowBlit(dst, reinterpret_cast<uint16_t *>(src), cw);

        if (g_is565 != 0) {
            uint16_t *d = dst;
            int16_t *s = src;
            for (int n = cw; n > 0; n--) {
                if (*s != 0)
                    *d = static_cast<uint16_t>(((*d & 0xffe0) << 1) | (*d & 0x1f));     // 555 -> 565
                s++;
                d++;
            }
        }

        dst += stride;
        rowIdx++;
        rows--;
    } while (rows != 0);
}
HOOK_FUNC(0x00433ed0, DrawGumpArt);

// 0x00434060  size 0x1f1
// Same as DrawGumpArt but clipped to the RECT clip as well.
// ECX = row blitter, EDX = art id; stack: clip, x, y, dest, stride, destH.
void __fastcall DrawGumpArtClipped(RowBlitFn rowBlit, int artId, const RECT *clip, int x, int y,
                                          uint16_t *dest, int stride, int destH)
{
    int16_t line[1024];
    int ok;
    unsigned dims;
    int *rowTable;
    int w, h, xskip, cw, right, bottom, yskip, lastRow, rows;
    uint16_t *dst;
    int *rowIdx;

    rowTable = Art_Load(g_artLoader, 0xb, 0xc, artId, &ok, &dims);
    if (ok == 0)
        return;

    h = static_cast<int>(dims & 0xffff);
    w = static_cast<int>(dims) >> 16;
    right = clip->right;
    if (x >= right)
        return;
    bottom = clip->bottom;
    if (y >= bottom)
        return;
    if (w + x < clip->left)
        return;
    int yEnd = h + y;
    if (yEnd < clip->top)
        return;

    if (clip->left > x)
        xskip = clip->left - x;
    else
        xskip = 0;

    if (right < w + x)
        cw = right - x;
    else
        cw = w;
    if (cw + x >= stride)
        cw = stride - x - 1;
    if (xskip >= cw)
        return;

    if (clip->top > y)
        yskip = clip->top - y;
    else
        yskip = 0;

    if (bottom < yEnd)
        lastRow = bottom - y - 1;
    else
        lastRow = h - 1;

    dst = dest + ((yskip + y) * stride + xskip + x);
    cw = cw - xskip;                  // span width passed to the row blitter
    if (lastRow + y >= destH)
        lastRow = destH - y - 1;
    if (yskip > lastRow)
        return;

    rows = lastRow - yskip + 1;
    rowIdx = rowTable + yskip;
    do {
        expand_row(line, rowTable + *rowIdx, w);

        int16_t *src = &line[xskip];
        rowBlit(dst, reinterpret_cast<uint16_t *>(src), cw);

        if (g_is565 != 0) {
            uint16_t *d = dst;
            int16_t *s = src;
            for (int n = cw; n > 0; n--) {
                if (*s != 0)
                    *d = static_cast<uint16_t>(((*d & 0xffe0) << 1) | (*d & 0x1f));     // 555 -> 565
                s++;
                d++;
            }
        }

        dst += stride;
        rowIdx++;
        rows--;
    } while (rows != 0);
}
HOOK_FUNC(0x00434060, DrawGumpArtClipped);
