// RLE sprite blitters of the world view (0x0042a670, 0x0042a7f0, 0x0042ac60). (Porting pass 2, batch 10.)
//
// Sprite stream: a sequence of records, each a 32-bit header followed by `len` bytes, ended by 0x7fff7fff.
//   bits 0..11   len, number of palette index bytes that follow the header
//   bits 12..21  signed row offset (the shadow stream stores `row * 2 + skip` here, see WorldDrawShadowRLE)
//   bits 22..31  signed column offset
// The next record starts at the header + 4 + len. Pixels are indices into a 16-bit palette. The destination
// is the 640-wide back buffer at 0x0069dd9c. All three procedures take ECX / EDX as the first two arguments
// and pop their stack arguments (MSVC __fastcall).
#include "world_renderer.h"

#include "../runtime/ported.h"
#include "globals.h"

#define g_hueTable GLOBAL_PTR(uint16_t, g_hueTable_storage)      // 0x2c entries per hue, indexed by the pixel's top 5 bits

static const int32_t kRleEnd = 0x7fff7fff;

static inline int32_t RleHeader(const uint8_t *p)
{
    return *reinterpret_cast<const int32_t *>(p);
}
static inline int RleCol(int32_t v)
{
    return v >> 22;
}
static inline int RleRow(int32_t v)
{
    return static_cast<int32_t>(static_cast<uint32_t>(v) << 10) >> 22;
}

// 0x0042a670  size 0x173
// ECX = mirror flag, EDX = x, stack (y, clip {left, top, right, bottom}, rle), RET 0xc.
// Halves the brightness of the back buffer pixels under each run, for the clipped part of the shadow.
void __fastcall WorldDrawShadowRLE(int mirror, int x, int y, const int *clip, const uint8_t *rle)
{
    int yb = y - g_scrollY;
    int xb = x - g_scrollX;
    const uint8_t *p = rle;
    for (;;) {
        int32_t v = RleHeader(p);
        if (v == kRleEnd)
            return;
        int len = v & 0xfff;
        int rowField = RleRow(v);
        if ((rowField & 1) != 0) {          // bit 12: skip record
            p = p + len + 4;
            continue;
        }
        int dx = rowField >> 1;
        int col;
        if (mirror != 0)
            col = xb - RleCol(v) - len - dx;
        else
            col = RleCol(v) + xb - dx;
        int row = yb + dx;
        p = p + 4;
        if (row >= clip[3])
            return;
        if (row >= clip[1]) {
            int i = 0;
            if (len > 0) {
                do {
                    if (col >= clip[2])
                        break;
                    if (col >= clip[0]) {
                        uint16_t *px = g_backBuffer + (row * 640 + col);
                        *px = static_cast<uint16_t>((*px >> 1) & 0x3def);
                    }
                    col++;
                    i++;
                } while (i < len);
            }
        }
        p = p + len;
    }
}
HOOK_FUNC(0x0042a670, WorldDrawShadowRLE);

// 0x0042a7f0  size 0x469
// ECX = mirror flag, EDX = hue mode, stack (x, y, clip, palette, rle), RET 0x14.
// mode & 0x7fff == 0: plain copy of palette[index].
// bit 15 clear: every pixel is replaced by hueTable[(pixel >> redShift) + (mode - 1) * 0x2c].
// bit 15 set: only grey pixels (pixel & 0x3ff == pixel >> 5) are replaced, by
//             hueTable[(pixel >> 10) + ((mode & 0x7fff) - 1) * 0x2c].
// The original has six copies of the loop (three modes, mirrored or not); they are one loop here.
void __fastcall WorldBlitRLESprite(int mirror, unsigned mode, int x, int y, const int *clip, const uint16_t *pal,
                                   const uint8_t *rle)
{
    int xb = x - g_scrollX;
    int yb = y - g_scrollY;
    const uint8_t *p = rle;
    for (;;) {
        int32_t v = RleHeader(p);
        if (v == kRleEnd)
            return;
        int len = v & 0xfff;
        int row = RleRow(v) + yb;
        int col;
        if (mirror != 0)
            col = xb - RleCol(v) - len;
        else
            col = RleCol(v) + xb;
        const uint8_t *data = p + 4;
        if (row >= clip[3])
            return;
        if (row >= clip[1]) {
            for (int i = 0; i < len; i++, col++) {
                if (col >= clip[2])
                    break;
                if (col < clip[0])
                    continue;
                unsigned idx = (mirror != 0) ? data[len - i - 1] : data[i];
                unsigned pix = pal[idx];
                unsigned out;
                if ((mode & 0x7fff) == 0) {
                    out = pix;
                } else if ((mode & 0x8000) == 0) {
                    int e = static_cast<int>(mode) - 1;
                    out = g_hueTable[(pix >> (GLOBAL_AS(uint8_t, g_redShift16) & 31)) + e * 0x2c];
                } else {
                    out = pix;
                    if ((pix & 0x3ff) == (pix >> 5)) {
                        int e = static_cast<int>(mode & 0x7fff) - 1;
                        out = g_hueTable[(pix >> 10) + e * 0x2c];
                    }
                }
                g_backBuffer[row * 640 + col] = static_cast<uint16_t>(out);
            }
        }
        p = data + len;
    }
}
HOOK_FUNC(0x0042a7f0, WorldBlitRLESprite);

// 0x0042ac60  size 0xb2
// ECX = px, EDX = py, stack (mirror, xorg, yorg, rle), RET 0x10. 1 when a run covers (px, py).
int __fastcall WorldHitTestRLESprite(int px, int py, int mirror, int xorg, int yorg, const uint8_t *rle)
{
    const uint8_t *p = rle;
    for (;;) {
        int32_t v = RleHeader(p);
        if (v == kRleEnd)
            return 0;
        int len = v & 0xfff;
        int row = RleRow(v) + yorg;
        int col;
        if (mirror != 0)
            col = xorg - RleCol(v) - len;
        else
            col = RleCol(v) + xorg;
        if (row > py)
            return 0;
        if (row == py && col <= px && col + len > px)
            return 1;
        p = p + len + 4;
    }
}
HOOK_FUNC(0x0042ac60, WorldHitTestRLESprite);
