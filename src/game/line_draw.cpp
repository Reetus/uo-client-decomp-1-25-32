// 16-bit line drawing on gump bitmaps. A bitmap is {uint16 width, uint16 height,
// pixels...}; the pixel at (x, y) is bmp[2 + y * width + x].
#include <cstdint>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"

// Pixel-op constants of the original image (RGB mask and add term).

static inline void shade_pixel(uint16_t *p)
{
    *p = static_cast<uint16_t>(((*p >> 1) & (g_blendMaskB >> 1)) + g_shadeAdd);
}

// 0x00432f90  size 0x354
// Darken/blend line from (x0, y0) to (x1, y1) into bmp. Always returns 1.
int __stdcall DrawLine16Shaded(int x0, int y0, int x1, int y1, uint16_t *bmp)
{
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
                shade_pixel(p);
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
                shade_pixel(p);
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
                shade_pixel(p);
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
                shade_pixel(p);
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
        w = bmp[0];
        p = &bmp[2 + (y0 * w + x0)];
        shade_pixel(p);
        if ((x0 & 0xffff) == x1)
            return 1;
        for (;;) {
            fy += slope;
            x += xstep;
            p = &bmp[2 + ((fy >> 16) * w + (x & 0xffff))];
            shade_pixel(p);
            if ((x & 0xffff) == x1)
                break;
        }
        return 1;
    } else {
        // y-major
        ystep = (y1 <= y0) ? -1 : 1;
        w = bmp[0];
        slope = (dx << 16) / ay;
        fx = x0 << 16;
        p = &bmp[2 + (y0 * w + x0)];
        shade_pixel(p);
        y = y0;
        if ((y0 & 0xffff) == y1)
            return 1;
        for (;;) {
            y += ystep;
            fx += slope;
            p = &bmp[2 + ((y & 0xffff) * w + (fx >> 16))];
            shade_pixel(p);
            if ((y & 0xffff) == y1)
                break;
        }
        return 1;
    }
}
HOOK_FUNC(0x00432f90, DrawLine16Shaded);

// 0x004332f0  size 0x2de
// Same five cases, but each pixel is copied from src (same index); the width
// comes from the destination bitmap only. Always returns 1.
int __stdcall DrawLine16FromSource(int x0, int y0, int x1, int y1, uint16_t *bmp, uint16_t *src)
{
    int w = bmp[0];
    int x, y, xstep, ystep, ax, ay, dx, dy, slope, fx, fy;
    uint16_t *p;

    if (x0 == x1) {
        // vertical
        if (y0 < y1) {
            p = &bmp[2 + (y0 * w + x0)];
            if ((y0 & 0xffff) > y1)
                return 1;
            y = y0;
            do {
                *p = src[2 + ((y & 0xffff) * w + x0)];
                p += w;
                y++;
            } while ((y & 0xffff) <= y1);
            return 1;
        } else {
            p = &bmp[2 + (y1 * w + x0)];
            if ((y1 & 0xffff) > y0)
                return 1;
            y = y1;
            do {
                *p = src[2 + ((y & 0xffff) * w + x0)];
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
                *p = src[2 + ((x & 0xffff) + y0 * w)];
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
                *p = src[2 + ((x & 0xffff) + y0 * w)];
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
        p = &bmp[2 + (y0 * w + x0)];
        *p = src[2 + (y0 * w + x0)];
        if ((x0 & 0xffff) == x1)
            return 1;
        for (;;) {
            fy += slope;
            x += xstep;
            int idx = (fy >> 16) * w + (x & 0xffff);
            bmp[2 + idx] = src[2 + idx];
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
        bmp[2 + (y0 * w + x0)] = src[2 + (y0 * w + x0)];
        if ((y0 & 0xffff) == y1)
            return 1;
        for (;;) {
            y += ystep;
            fx += slope;
            int idx = (y & 0xffff) * w + (fx >> 16);
            bmp[2 + idx] = src[2 + idx];
            if ((y & 0xffff) == y1)
                break;
        }
        return 1;
    }
}
HOOK_FUNC(0x004332f0, DrawLine16FromSource);
