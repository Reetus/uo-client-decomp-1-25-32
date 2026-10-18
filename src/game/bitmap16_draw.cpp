// 16-bit gump bitmap drawing: rectangle fill and plain line (0x00432c70, 0x00432cf0).
// A bitmap is {uint16 width, uint16 height, pixels...}; the pixel at (x, y) is bmp[2 + y * width + x].
// The shaded and copying variants of the line routine are in line_draw.cpp.
#include <cstdint>

#include "../runtime/ported.h"

// 0x00432c70  size 0x7b
// Fill the rectangle x0..x1, y0..y1 (inclusive) of `bmp` with `color`, two pixels at a time:
// the start is the dword at ((y0 >> 1) * width + (x0 >> 1)), each row writes (x1 - x0 + 1) >> 1
// dwords and then steps by (width >> 1) dwords from its start, so odd x0 / x1 are rounded.
// Always returns 1.
int __stdcall Bitmap16_FillRect(int x0, int y0, int x1, int y1, int color, uint16_t *bmp)
{
    uint32_t pair = static_cast<uint32_t>(color & 0xffff);
    pair = (pair << 16) + static_cast<uint32_t>(color & 0xffff);
    uint32_t *dst = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(bmp) + 4) + ((y0 >> 1) * static_cast<int>(bmp[0]) + (x0 >> 1));
    int rows = y1 - y0 + 1;

    if (rows > 0) {
        int count = (x1 - x0 + 1) >> 1;
        do {
            if (count > 0) {
                for (int i = 0; i < count; i++)
                    dst[i] = pair;
                dst += count;
            }
            dst += static_cast<int>(bmp[0] >> 1) - count;
            rows--;
        } while (rows != 0);
    }
    return 1;
}
HOOK_FUNC(0x00432c70, Bitmap16_FillRect);

// 0x00432cf0  size 0x294
// Line from (x0, y0) to (x1, y1) in `color`. Vertical and horizontal lines are walked pixel by
// pixel from the smaller end (16-bit wrapped counters); otherwise the major axis steps by one and
// the other coordinate advances in 16.16 fixed point. Always returns 1.
int __stdcall DrawLine16(int x0, int y0, int x1, int y1, int color, uint16_t *bmp)
{
    uint16_t pixel = static_cast<uint16_t>(color);
    int w = bmp[0];
    int x, y, xstep, ystep, ax, ay, dx, dy, slope, fx, fy;
    uint16_t *p;

    if (x0 == x1) {
        // vertical
        if (y0 < y1) {
            p = &bmp[2 + (y0 * w + x0)];
            y = y0;
            if ((y0 & 0xffff) > y1)
                return 1;
            do {
                *p = pixel;
                p += w;
                y++;
            } while ((y & 0xffff) <= y1);
            return 1;
        } else {
            p = &bmp[2 + (y1 * w + x0)];
            y = y1;
            if ((y1 & 0xffff) > y0)
                return 1;
            do {
                *p = pixel;
                p += w;
                y++;
            } while ((y & 0xffff) <= y0);
            return 1;
        }
    }

    if (y0 == y1) {
        // horizontal
        if (x0 < x1) {
            x = x0;
            p = &bmp[2 + (y0 * w + x0)];
            if ((x0 & 0xffff) > x1)
                return 1;
            do {
                *p = pixel;
                p++;
                x++;
            } while ((x & 0xffff) <= x1);
            return 1;
        } else {
            x = x1;
            p = &bmp[2 + (y0 * w + x1)];
            if ((x1 & 0xffff) > x0)
                return 1;
            do {
                *p = pixel;
                p++;
                x++;
            } while ((x & 0xffff) <= x0);
            return 1;
        }
    }

    dx = x1 - x0;
    ax = dx < 0 ? -dx : dx;
    dy = y1 - y0;
    ay = dy < 0 ? -dy : dy;

    if (ax > ay) {
        // x-major
        xstep = (x1 <= x0) ? -1 : 1;
        slope = (dy << 16) / ax;
        fy = y0 << 16;
        x = x0;
        bmp[2 + (y0 * w + x0)] = pixel;
        if ((x0 & 0xffff) == x1)
            return 1;
        for (;;) {
            fy += slope;
            x += xstep;
            bmp[2 + ((fy >> 16) * w + (x & 0xffff))] = pixel;
            if ((x & 0xffff) == x1)
                break;
        }
        return 1;
    } else {
        // y-major
        ystep = (y1 <= y0) ? -1 : 1;
        slope = (dx << 16) / ay;
        fx = x0 << 16;
        y = y0;
        bmp[2 + (y0 * w + x0)] = pixel;
        if ((y0 & 0xffff) == y1)
            return 1;
        for (;;) {
            y += ystep;
            fx += slope;
            bmp[2 + ((y & 0xffff) * w + (fx >> 16))] = pixel;
            if ((y & 0xffff) == y1)
                break;
        }
        return 1;
    }
}
HOOK_FUNC(0x00432cf0, DrawLine16);
