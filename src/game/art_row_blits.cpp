// Row blit routines of the art blitter (0x00476410-0x00476710), the surface
// setter 0x004767c0 and the unclipped RLE walker 0x00476990.
// Row routines: ECX = dst, EDX = src, one stack argument = pixel count (RET 4),
// i.e. __fastcall(dst, src, count). A zero count returns at once; otherwise
// the loop runs count times (a negative count wraps like the original).
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

#define g_colorTable GLOBAL_PTR(uint16_t, g_hueTable_storage)

typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);

// True when the high part of the pixel equals its low 10 bits (the "keyed" case that is looked up).
static inline uint16_t RB_KeyDiff(uint16_t px)
{
    uint16_t t = px;
    t >>= GLOBAL_AS(uint16_t, g_greenShift16);
    t ^= px & 0x3ff;
    return t;
}

static inline uint16_t RB_Lookup(uint16_t px)
{
    unsigned idx = (static_cast<unsigned>(px) >> GLOBAL_AS(uint8_t, g_redShift16)) + static_cast<unsigned>(g_hueOverride * 44);
    return g_colorTable[idx];
}

static inline uint16_t RB_Blend(uint16_t dst, int src)
{
    int t = ((g_blendMaskA & src) + (g_blendMaskA & dst)) >> 1;
    t = (t + (g_blendMaskB & src)) >> 1;
    return static_cast<uint16_t>(t) | 1;
}

// 0x00476410  size 0xa7  keyed pixels are looked up and blended, others copied; no transparent test
void __fastcall RowBlitKeyedBlend(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        src++;
        if (RB_KeyDiff(px) != 0) {
            *dst = px;
        } else {
            uint16_t v = RB_Lookup(px);
            *dst = RB_Blend(*dst, v);
        }
        dst++;
    }
}
HOOK_FUNC(0x00476410, RowBlitKeyedBlend);

// 0x004764c0  size 0x2a  copy the non-zero source pixels
void __fastcall RowBlitCopyNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        if (px != 0)
            *dst = px;
        src++;
        dst++;
    }
}
HOOK_FUNC(0x004764c0, RowBlitCopyNonZero);

// 0x004764f0  size 0x5e  blend the non-zero source pixels over the destination
void __fastcall RowBlitBlendNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        if (px != 0)
            *dst = RB_Blend(*dst, px);
        src++;
        dst++;
    }
}
HOOK_FUNC(0x004764f0, RowBlitBlendNonZero);

// 0x00476550  size 0x4b  non-zero source: ((m0 & src) + (m0 & dst)) >> 1 | 1
void __fastcall RowBlitHalfMaskNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        if (px != 0) {
            int t = (g_blendMaskB & static_cast<int>(px)) + (static_cast<int>(*dst) & g_blendMaskB);
            t >>= 1;
            *dst = static_cast<uint16_t>(t) | 1;
        }
        src++;
        dst++;
    }
}
HOOK_FUNC(0x00476550, RowBlitHalfMaskNonZero);

// 0x004765a0  size 0x54  non-zero source pixels are replaced by their table entry
void __fastcall RowBlitLookupNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        if (px != 0)
            *dst = RB_Lookup(px);
        src++;
        dst++;
    }
}
HOOK_FUNC(0x004765a0, RowBlitLookupNonZero);

// 0x00476600  size 0x74  non-zero source: copied unless keyed, in which case the table entry is stored
void __fastcall RowBlitKeyedLookupNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        src++;
        if (px != 0) {
            if (RB_KeyDiff(px) != 0)
                *dst = px;
            else
                *dst = RB_Lookup(px);
        }
        dst++;
    }
}
HOOK_FUNC(0x00476600, RowBlitKeyedLookupNonZero);

// 0x00476680  size 0x84  non-zero source: table entry blended over the destination
void __fastcall RowBlitLookupBlendNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        if (px != 0) {
            uint16_t v = RB_Lookup(px);
            *dst = RB_Blend(*dst, v);
        }
        src++;
        dst++;
    }
}
HOOK_FUNC(0x00476680, RowBlitLookupBlendNonZero);

// 0x00476710  size 0xac  non-zero source: copied unless keyed, in which case the table entry is blended
void __fastcall RowBlitKeyedLookupBlendNonZero(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        src++;
        if (px != 0) {
            if (RB_KeyDiff(px) != 0) {
                *dst = px;
            } else {
                uint16_t v = RB_Lookup(px);
                *dst = RB_Blend(*dst, v);
            }
        }
        dst++;
    }
}
HOOK_FUNC(0x00476710, RowBlitKeyedLookupBlendNonZero);

// 0x004767c0  size 0xd  set the destination surface (ECX) and its row stride in pixels (EDX)
void __fastcall SetBlitSurface(uint16_t *dest, int stride)
{
    g_blitDest = dest;
    g_blitStride = stride;
}
HOOK_FUNC(0x004767c0, SetBlitSurface);

// 0x00476990  size 0xd3  walk an RLE art record unclipped at (x, y), calling the row-blit function per run
// ECX = row-blit (ECX = dest, EDX = src, stack = count), EDX = art record, stack = (x, y), RET 8.
// Record: +6 height, +8 ushort rowOffset[height], then run data; each row = {ushort startX, then
// repeat {ushort len; len pixels; ushort gap}} until len == 0.
void __fastcall RleArtWalkUnclipped(RowBlitFn blit, uint8_t *rec, int x, int y)
{
    if (rec == nullptr)
        return;

    int height = *reinterpret_cast<uint16_t *>(rec + 6);
    uint16_t *data = reinterpret_cast<uint16_t *>(rec + height * 2 + 8);
    uint16_t *rowOff = reinterpret_cast<uint16_t *>(rec + 8);
    uint16_t *dstRow = g_blitDest + (y * g_blitStride + x);
    if (height <= 0)
        return;

    int rows = height;
    do {
        uint16_t *p = data + *rowOff;
        uint16_t cur = p[0];
        uint16_t *src = p + 1;
        uint16_t len = *src;
        src++;
        if (len != 0) {
            do {
                blit(dstRow + cur, src, len);
                src += len;
                uint32_t gap = *src;
                src++;
                cur += gap + len;
                len = *src;
                src++;
            } while (len != 0);
        }
        dstRow += g_blitStride;
        rowOff++;
        rows--;
    } while (rows != 0);
}
HOOK_FUNC(0x00476990, RleArtWalkUnclipped);
