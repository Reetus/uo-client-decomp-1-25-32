// Row routines of the art blitter that run over every pixel (no transparent test): 0x004761e0 .. 0x004762d0
// and 0x00476390. Each is passed by address as a callback: ECX = dst, EDX = src, one stack argument = pixel
// count (RET 4), i.e. __fastcall(dst, src, count). A zero count returns at once (not 0x004761e0); the loops
// run `count` times, so a negative count wraps like the original. The non-zero-only routines are in
// art_row_blits.cpp.
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"

#define g_colorTable GLOBAL_PTR(uint16_t, g_hueTable_storage)

// The high part of the pixel equals its low 10 bits (zero when keyed).
static inline uint16_t RBH_KeyDiff(uint16_t px)
{
    uint16_t t = px;
    t >>= GLOBAL_AS(uint16_t, g_greenShift16);
    t ^= px & 0x3ff;
    return t;
}

static inline uint16_t RBH_Lookup(uint16_t px)
{
    unsigned idx = static_cast<unsigned>(g_hueOverride * 44) + (static_cast<unsigned>(px) >> GLOBAL_AS(uint8_t, g_redShift16));
    return g_colorTable[idx];
}

static inline int RBH_Blend(uint16_t src, uint16_t dst)
{
    int t = ((g_blendMaskA & src) + (dst & g_blendMaskA)) >> 1;
    t = (t + (g_blendMaskB & src)) >> 1;
    return t | 1;
}
// 0x004761e0  size 0x23  copy `count` pixels (count * 2 bytes; no zero test)
void __fastcall RowBlitCopyAll(uint16_t *dst, uint16_t *src, int count)
{
    memcpy(dst, src, static_cast<size_t>(static_cast<uint32_t>(count) * 2u));
}
HOOK_FUNC(0x004761e0, RowBlitCopyAll);

// 0x00476210  size 0x4e  every pixel is replaced by its table entry
void __fastcall RowBlitLookupAll(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        dst++;
        dst[-1] = RBH_Lookup(*src);
        src++;
    }
}
HOOK_FUNC(0x00476210, RowBlitLookupAll);

// 0x00476260  size 0x6f  keyed pixels are replaced by their table entry, others are copied
void __fastcall RowBlitKeyedLookupAll(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        src++;
        if (RBH_KeyDiff(px) != 0)
            *dst = px;
        else
            *dst = RBH_Lookup(px);
        dst++;
    }
}
HOOK_FUNC(0x00476260, RowBlitKeyedLookupAll);

// 0x004762d0  size 0x55  every pixel is blended 50% over the destination
void __fastcall RowBlitBlendAll(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t px = *src;
        uint16_t d = *dst;
        src++;
        dst++;
        dst[-1] = RBH_Blend(px, d);
    }
}
HOOK_FUNC(0x004762d0, RowBlitBlendAll);

// 0x00476390  size 0x7a  every pixel is replaced by its table entry and blended 50% over the destination
void __fastcall RowBlitLookupBlendAll(uint16_t *dst, uint16_t *src, int count)
{
    while (count-- != 0) {
        uint16_t v = RBH_Lookup(*src);
        src++;
        dst++;
        dst[-1] = RBH_Blend(v, dst[-1]);
    }
}
HOOK_FUNC(0x00476390, RowBlitLookupBlendAll);
