// uV intro video player, header / palette reader, motion-compensated block copies
// and the macroblock walkers for the DCT frames (porting pass 3, batch v2):
// 0x00485460, 0x004855c0, 0x004856c0, 0x004857a0, 0x004858a0, 0x00485aa0, 0x00485bb0.
//
// The walkers take the player in ECX and the macroblock stream in EDX and two
// plain surface pointers plus the pitch on the stack (RET 0xC; the second
// pointer is never read). The two flat-colour fills 0x004858a0 / 0x00485bb0
// get their first two bytes in CL / DL and four more bytes on the stack
// (the callers only set the low byte of each register; the callee sign-extends
// the low bytes), then the destination and the pitch.
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "uv_video.h"
#include "uv_dct_mb_ports.h"
#include "globals.h"

#define uv_scale UVD_Quant[0]          // the brightness-scaled first table entry

// 0x00487f71 / 0x00487792: DCT macroblock decoders, (stream, dst, pitch), cdecl, all registers preserved.
#define UV_DecodeBlocks16 PORT_FUNC3(&Thunk_UvDecodeBlocksS16, void, __cdecl, const uint8_t *, uint8_t *, int32_t)
#define UV_DecodeBlocks32 PORT_FUNC3(&Thunk_UvDecodeBlocksS32, void, __cdecl, const uint8_t *, uint8_t *, int32_t)

// imul r/m32 followed by "shl edx,16; shr eax,16; adc eax,edx": the 64-bit product shifted right by
// 16 with the last bit shifted out added back.
static inline int32_t uv_fmul16(int32_t a, int32_t b)
{
    int64_t prod = static_cast<int64_t>(a) * static_cast<int64_t>(b);
    uint32_t r = static_cast<uint32_t>(prod >> 16) + static_cast<uint32_t>((prod >> 15) & 1);
    return static_cast<int32_t>(r);
}

static inline uint16_t uv_rd16(const uint8_t *p)
{
    uint16_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

// byte * brightness / 100 through the compiler's reciprocal multiply (unsigned), then the 255 clamp.
static inline uint32_t uv_scale_channel(uint8_t value, uint32_t brightness)
{
    uint32_t t = static_cast<uint32_t>(value) * brightness;
    uint32_t hi = static_cast<uint32_t>((static_cast<uint64_t>(0x51eb851fu) * t) >> 32);
    uint32_t v = hi >> 5;
    if (v > 0xff)
        v = 0xff;
    return v;
}

// 0x00485460  size 0x157  Reads the stream header (width, height, two words) and the palette, scaled by the brightness.
void __fastcall UV_ReadHeaderPalette(UvVideo *o, const uint8_t *data)
{
    o->surf->width = uv_rd16(data);
    o->surf->height = uv_rd16(data + 2);
    o->hdr = uv_rd16(data + 4);
    int32_t count = uv_rd16(data + 6);
    o->palCount = count;
    int32_t i = 0;
    if (count > 0) {
        uint8_t *dst = o->palette;
        const uint8_t *src = data + 0xc;
        do {
            uint32_t bright = static_cast<uint32_t>(o->surf->brightness);
            uint32_t c0 = uv_scale_channel(src[0], bright);
            uint32_t c1 = uv_scale_channel(src[1], bright);
            uint32_t c2 = uv_scale_channel(src[2], bright);
            src += 3;
            dst[0] = static_cast<uint8_t>(c0);
            dst[1] = static_cast<uint8_t>(c1);
            dst[2] = static_cast<uint8_t>(c2);
            dst += 3;
            i++;
        } while (i < o->palCount);
    }
}
HOOK_FUNC(0x00485460, UV_ReadHeaderPalette);

// 0x004855c0  size 0xf2  Copies a 16x16 block of 16-bit pixels from the other buffer, displaced by a motion vector
// (a nibble above 7 is negative: minus 16). dst = ECX, src = EDX, (x, y, width, dx, dy) on the stack.
void __fastcall UV_CopyMotionBlock16(uint16_t *dst, const uint16_t *src, int32_t x, int32_t y, int32_t width, int32_t dx, int32_t dy)
{
    if (dx > 7)
        dx -= 0x10;
    if (dy > 7)
        dy -= 0x10;
    int32_t dstIndex = width * y + x;
    int32_t srcIndex = (y - dy) * width - dx + x;
    uint16_t *d = dst + dstIndex;
    const uint16_t *s = src + srcIndex;
    for (int32_t row = 0x10; row != 0; row--) {
        for (int32_t k = 0; k < 16; k++)
            d[k] = s[k];
        d += width;
        s += width;
    }
}
HOOK_FUNC(0x004855c0, UV_CopyMotionBlock16);

// 0x004856c0  size 0xd2  The same for 32-bit pixels.
void __fastcall UV_CopyMotionBlock32(uint32_t *dst, const uint32_t *src, int32_t x, int32_t y, int32_t width, int32_t dx, int32_t dy)
{
    if (dx > 7)
        dx -= 0x10;
    if (dy > 7)
        dy -= 0x10;
    int32_t dstIndex = width * y + x;
    int32_t srcIndex = (y - dy) * width - dx + x;
    uint32_t *d = dst + dstIndex;
    const uint32_t *s = src + srcIndex;
    for (int32_t row = 0x10; row != 0; row--) {
        for (int32_t k = 0; k < 16; k++)
            d[k] = s[k];
        d += width;
        s += width;
    }
}
HOOK_FUNC(0x004856c0, UV_CopyMotionBlock32);

// 0x004858a0  size 0x1f9  Fills a 16x16 block of 16-bit pixels as four 8x8 flat quadrants. CL, DL and the first two
// stack bytes are the four luma values, the next two the chroma indices (all signed bytes).
void __fastcall UV_FillQuads16(int8_t y0, int8_t y1, int8_t y2, int8_t y3, int8_t cr, int8_t cb, uint8_t *dst, uint32_t pitch)
{
    int32_t a = uv_fmul16(uv_scale, cr);
    int32_t b = uv_fmul16(uv_scale, cb);
    b >>= 1;
    a >>= 1;
    b &= 0x7f;
    a &= 0x7f;
    uint32_t base = uv_tab_cb_b[b] + uv_tab_cr_g[a];
    uint32_t pix = uv_tab_y[(uv_fmul16(uv_scale, y0) >> 1) & 0x7f] + base;
    uint32_t rowStep = (pitch >> 1) * 2 - 0x10;
    uint8_t *row = dst;
    for (int32_t n = 8; n != 0; n--) {
        uint32_t two = (static_cast<uint32_t>(static_cast<uint16_t>(pix)) << 16) | static_cast<uint16_t>(pix);
        memcpy(row, &two, 4);
        memcpy(row + 4, &two, 4);
        memcpy(row + 8, &two, 4);
        memcpy(row + 12, &two, 4);
        row += 0x10 + rowStep;
    }
    pix = uv_tab_y[(uv_fmul16(uv_scale, y1) >> 1) & 0x7f] + base;
    row = dst + 0x10;
    for (int32_t n = 8; n != 0; n--) {
        uint32_t two = (static_cast<uint32_t>(static_cast<uint16_t>(pix)) << 16) | static_cast<uint16_t>(pix);
        memcpy(row, &two, 4);
        memcpy(row + 4, &two, 4);
        memcpy(row + 8, &two, 4);
        memcpy(row + 12, &two, 4);
        row += 0x10 + rowStep;
    }
    pix = uv_tab_y[(uv_fmul16(uv_scale, y2) >> 1) & 0x7f] + base;
    row = dst + pitch * 8;
    for (int32_t n = 8; n != 0; n--) {
        uint32_t two = (static_cast<uint32_t>(static_cast<uint16_t>(pix)) << 16) | static_cast<uint16_t>(pix);
        memcpy(row, &two, 4);
        memcpy(row + 4, &two, 4);
        memcpy(row + 8, &two, 4);
        memcpy(row + 12, &two, 4);
        row += 0x10 + rowStep;
    }
    pix = uv_tab_y[(uv_fmul16(uv_scale, y3) >> 1) & 0x7f] + base;
    row = dst + pitch * 8 + 0x10;
    for (int32_t n = 8; n != 0; n--) {
        uint32_t two = (static_cast<uint32_t>(static_cast<uint16_t>(pix)) << 16) | static_cast<uint16_t>(pix);
        memcpy(row, &two, 4);
        memcpy(row + 4, &two, 4);
        memcpy(row + 8, &two, 4);
        memcpy(row + 12, &two, 4);
        row += 0x10 + rowStep;
    }
}
HOOK_FUNC(0x004858a0, UV_FillQuads16);

// 0x00485bb0  size 0x1b3  The same for 32-bit pixels (8 dwords per row).
void __fastcall UV_FillQuads32(int8_t y0, int8_t y1, int8_t y2, int8_t y3, int8_t cr, int8_t cb, uint8_t *dst, uint32_t pitch)
{
    int32_t a = uv_fmul16(uv_scale, cr);
    int32_t b = uv_fmul16(uv_scale, cb);
    b >>= 1;
    a >>= 1;
    b &= 0x7f;
    a &= 0x7f;
    uint32_t base = uv_tab_cr_g[a] + uv_tab_cb_b[b];
    uint32_t rowStep = (pitch >> 2) * 4 - 0x20;
    uint32_t pix = uv_tab_y[(uv_fmul16(uv_scale, y0) >> 1) & 0x7f] + base;
    uint8_t *row = dst;
    for (int32_t n = 8; n != 0; n--) {
        for (int32_t k = 0; k < 8; k++)
            memcpy(row + k * 4, &pix, 4);
        row += 0x20 + rowStep;
    }
    pix = uv_tab_y[(uv_fmul16(uv_scale, y1) >> 1) & 0x7f] + base;
    row = dst + 0x20;
    for (int32_t n = 8; n != 0; n--) {
        for (int32_t k = 0; k < 8; k++)
            memcpy(row + k * 4, &pix, 4);
        row += 0x20 + rowStep;
    }
    pix = uv_tab_y[(uv_fmul16(uv_scale, y2) >> 1) & 0x7f] + base;
    row = dst + pitch * 8;
    for (int32_t n = 8; n != 0; n--) {
        for (int32_t k = 0; k < 8; k++)
            memcpy(row + k * 4, &pix, 4);
        row += 0x20 + rowStep;
    }
    pix = uv_tab_y[(uv_fmul16(uv_scale, y3) >> 1) & 0x7f] + base;
    row = dst + pitch * 8 + 0x20;
    for (int32_t n = 8; n != 0; n--) {
        for (int32_t k = 0; k < 8; k++)
            memcpy(row + k * 4, &pix, 4);
        row += 0x20 + rowStep;
    }
}
HOOK_FUNC(0x00485bb0, UV_FillQuads32);

// 0x004857a0  size 0x100  Walks the macroblocks of a 16-bit frame. Each starts with a byte count; larger than 12 is a
// coded block, 3 / 6 / 12 are flat-colour blocks (4 luma + 2 chroma values, replicated for 3 and 6).
void __fastcall UV_WalkMacroblocks16(UvVideo *o, const uint8_t *src, uint8_t *dst, uint8_t * /*unused*/, uint32_t pitch)
{
    UvSurf *p = o->surf;
    uint8_t *rowBase = dst;
    uint32_t y = 0;
    if (p->height == 0)
        return;
    do {
        uint32_t x = 0;
        if (p->width != 0) {
            do {
                uint32_t count = *src++;
                if (static_cast<int32_t>(count) > 0xc) {
                    UV_DecodeBlocks16(src, dst, static_cast<int32_t>(pitch));
                } else if (count == 3) {
                    UV_FillQuads16(static_cast<int8_t>(src[0]), static_cast<int8_t>(src[0]), static_cast<int8_t>(src[0]),
                                   static_cast<int8_t>(src[0]), static_cast<int8_t>(src[1]), static_cast<int8_t>(src[2]), dst, pitch);
                } else if (count == 6) {
                    UV_FillQuads16(static_cast<int8_t>(src[0]), static_cast<int8_t>(src[1]), static_cast<int8_t>(src[2]),
                                   static_cast<int8_t>(src[3]), static_cast<int8_t>(src[4]), static_cast<int8_t>(src[5]), dst, pitch);
                } else if (count == 0xc) {
                    UV_FillQuads16(static_cast<int8_t>(src[0]), static_cast<int8_t>(src[2]), static_cast<int8_t>(src[4]),
                                   static_cast<int8_t>(src[6]), static_cast<int8_t>(src[8]), static_cast<int8_t>(src[10]), dst, pitch);
                }
                src += count;
                x += 0x10;
                dst += 0x20;
            } while (x < o->surf->width);
        }
        dst = rowBase + ((pitch >> 1) << 5);
        rowBase = dst;
        y += 0x10;
        p = o->surf;
    } while (y < p->height);
}
HOOK_FUNC(0x004857a0, UV_WalkMacroblocks16);

// 0x00485aa0  size 0x101  The same for a 32-bit frame (dst advances 0x40 bytes per macroblock).
void __fastcall UV_WalkMacroblocks32(UvVideo *o, const uint8_t *src, uint8_t *dst, uint8_t * /*unused*/, uint32_t pitch)
{
    UvSurf *p = o->surf;
    uint8_t *rowBase = dst;
    uint32_t y = 0;
    if (p->height == 0)
        return;
    do {
        uint32_t x = 0;
        if (p->width != 0) {
            do {
                uint32_t count = *src++;
                if (static_cast<int32_t>(count) > 0xc) {
                    UV_DecodeBlocks32(src, dst, static_cast<int32_t>(pitch));
                } else if (count == 3) {
                    UV_FillQuads32(static_cast<int8_t>(src[0]), static_cast<int8_t>(src[0]), static_cast<int8_t>(src[0]),
                                   static_cast<int8_t>(src[0]), static_cast<int8_t>(src[1]), static_cast<int8_t>(src[2]), dst, pitch);
                } else if (count == 6) {
                    UV_FillQuads32(static_cast<int8_t>(src[0]), static_cast<int8_t>(src[1]), static_cast<int8_t>(src[2]),
                                   static_cast<int8_t>(src[3]), static_cast<int8_t>(src[4]), static_cast<int8_t>(src[5]), dst, pitch);
                } else if (count == 0xc) {
                    UV_FillQuads32(static_cast<int8_t>(src[0]), static_cast<int8_t>(src[2]), static_cast<int8_t>(src[4]),
                                   static_cast<int8_t>(src[6]), static_cast<int8_t>(src[8]), static_cast<int8_t>(src[10]), dst, pitch);
                }
                src += count;
                x += 0x10;
                dst += 0x40;
            } while (x < o->surf->width);
        }
        dst = rowBase + ((pitch >> 2) << 6);
        rowBase = dst;
        y += 0x10;
        p = o->surf;
    } while (y < p->height);
}
HOOK_FUNC(0x00485aa0, UV_WalkMacroblocks32);
