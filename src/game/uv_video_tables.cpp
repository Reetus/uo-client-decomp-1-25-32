// uV intro video player, pixel-format conversion tables (porting pass 3, batch v2):
// the builder 0x00485da0 (ECX = player) and its colour packer 0x00486350.
#include <cstdint>

#include "../runtime/ported.h"
#include "uv_video.h"
#include "globals.h"


// "shl r, cl" / "sar r, cl": the count is taken mod 32 by the hardware.
static inline int32_t uv_shl(int32_t v, int32_t count)
{
    return static_cast<int32_t>(static_cast<uint32_t>(v) << (count & 31));
}

static inline int32_t uv_sar(int32_t v, int32_t count)
{
    return v >> (count & 31);
}

// imul r/m32 then "shl edx,16; shr eax,16; adc eax,edx": the 64-bit product shifted right by 16, last bit added back.
static inline int32_t uv_fmul16(int32_t a, int32_t b)
{
    int64_t prod = static_cast<int64_t>(a) * static_cast<int64_t>(b);
    uint32_t r = static_cast<uint32_t>(prod >> 16) + static_cast<uint32_t>((prod >> 15) & 1);
    return static_cast<int32_t>(r);
}

// Division by 100 as the compiler's "imul 0x51eb851f; sar edx,5; add sign bit".
static inline int32_t uv_div100(int32_t v)
{
    int64_t prod = static_cast<int64_t>(0x51eb851f) * static_cast<int64_t>(v);
    int32_t hi = static_cast<int32_t>(prod >> 32);
    hi >>= 5;
    hi = static_cast<int32_t>(static_cast<uint32_t>(hi) + (static_cast<uint32_t>(hi) >> 31));
    return hi;
}

// 0x00486350  size 0x233  Packs three components into two 16-bit pixels (EAX = low pixel quantised, high half rounded
// toward zero), in the display's 15 / 16-bit channel layout. A = ECX (red), B = EDX (green), x (blue) on the stack;
// the second stack argument is never read; then the bit counts and the shifts.
static uint32_t __fastcall UV_PackColor16(int32_t a, int32_t b, int32_t x, int32_t /*unused*/, int32_t rb, int32_t gb, int32_t bb,
                                          int32_t rs, int32_t gs, int32_t bs)
{
    int32_t stepR = uv_shl(1, 8 - rb);
    int32_t qA = a / stepR;
    int32_t rQ;
    int32_t rLim;
    if (a >= 0) {
        rQ = (uv_shl(1, 7 - rb) + a) / stepR;
        rLim = uv_shl(1, rb);
        if (rQ > rLim - 1)
            rQ = rLim - 1;
    } else {
        rQ = (a - uv_shl(1, 7 - rb)) / stepR;
        rLim = uv_shl(1, rb);
        if (rQ < 1 - rLim)
            rQ = 1 - rLim;
    }
    int32_t stepG = uv_shl(1, 8 - gb);
    int32_t qB = b / stepG;
    int32_t gQ;
    int32_t gLim;
    if (b >= 0) {
        gQ = (uv_shl(1, 7 - gb) + b) / stepG;
        gLim = uv_shl(1, gb);
        if (gQ > gLim - 1)
            gQ = gLim - 1;
    } else {
        gQ = (b - uv_shl(1, 7 - gb)) / stepG;
        gLim = uv_shl(1, gb);
        if (gQ < 1 - gLim)
            gQ = 1 - gLim;
    }
    int32_t stepB = uv_shl(1, 8 - bb);
    int32_t qX = x / stepB;
    int32_t bQ;
    int32_t bLim;
    if (x >= 0) {
        bQ = (uv_shl(1, 7 - bb) + x) / stepB;
        bLim = uv_shl(1, bb);
        if (bQ > bLim - 1)
            bQ = bLim - 1;
    } else {
        bQ = (x - uv_shl(1, 7 - bb)) / stepB;
        bLim = uv_shl(1, bb);
        if (bQ < 1 - bLim)
            bQ = 1 - bLim;
    }
    if (bQ < 0)
        gQ--;
    if (gQ < 0)
        rQ--;
    if (rQ < 0)
        qX--;
    if (qX < 0)
        qB--;
    if (qB < 0)
        qA--;
    int32_t maskR = rLim - 1;
    int32_t maskG = gLim - 1;
    int32_t maskB = bLim - 1;
    int32_t res = uv_shl(qB & maskG, gs + 0x10);
    res |= uv_shl(qA & maskR, rs + 0x10);
    res |= uv_shl(qX & maskB, bs + 0x10);
    res |= uv_shl(rQ & maskR, rs);
    res |= uv_shl(gQ & maskG, gs);
    res |= uv_shl(bQ & maskB, bs);
    if (bb + gb + rb != 0x10)
        res |= static_cast<int32_t>(0x80008000u);
    return static_cast<uint32_t>(res);
}
HOOK_FUNC(0x00486350, UV_PackColor16);

// 0x00485da0  size 0x5a3  Builds the YUV-to-display lookup tables for the display's pixel format.
void __fastcall UV_BuildTables(UvVideo *o)
{
    UvSurf *p = o->surf;
    int32_t bpp = p->bpp;
    if (bpp != 0x10 && bpp != 0xf) {
        // Generic layout: packed bytes (table values: 0xff in the top byte, the component repeated).
        for (int32_t i = 0; i < 0x80; i++) {
            int8_t al = static_cast<int8_t>(static_cast<uint8_t>(i) * 2 + 1);
            int32_t ecx = static_cast<int32_t>(al) + 0x80;
            int32_t edx = ecx;
            int32_t eax = ecx;
            int32_t esi = 0xff;
            if (ecx < 0)
                edx = ecx - 1;
            if (edx < 0)
                eax = ecx - 1;
            if (eax < 0)
                esi = 0xfe;
            eax &= 0xff;
            edx &= 0xff;
            esi <<= 8;
            eax |= esi;
            ecx &= 0xff;
            eax <<= 8;
            eax |= edx;
            eax <<= 8;
            eax |= ecx;
            uv_tab_y[i] = static_cast<uint32_t>(eax);
        }
        for (int32_t i = 0; i < 0x80; i++) {
            int8_t cl = static_cast<int8_t>(static_cast<uint8_t>(i) * 2 + 1);
            int32_t ecx = cl;
            int32_t ebx = uv_fmul16(static_cast<int32_t>(0xffffa7ea), ecx);
            int32_t eax = uv_fmul16(0x1c5a1, ecx);
            int32_t esi = 0;
            ecx = 0;
            int32_t edx = ebx;
            if (eax < 0)
                edx = ebx - 1;
            if (edx < 0)
                ecx = -1;
            if (ecx < 0)
                esi = -1;
            ecx &= 0xff;
            edx &= 0xff;
            esi <<= 8;
            ecx |= esi;
            eax &= 0xff;
            ecx <<= 8;
            ecx |= edx;
            ecx <<= 8;
            ecx |= eax;
            uv_tab_cr_g[i] = static_cast<uint32_t>(ecx);
        }
        for (int32_t i = 0; i < 0x80; i++) {
            int8_t dl = static_cast<int8_t>(static_cast<uint8_t>(i) * 2 + 1);
            int32_t esi = dl;
            int32_t ecx = uv_fmul16(0x166e9, esi);
            int32_t eax = uv_fmul16(static_cast<int32_t>(0xffff4931), esi);
            int32_t edx = 0;
            if (eax < 0)
                ecx--;
            if (ecx < 0)
                edx = -1;
            ecx &= 0xff;
            eax &= 0xff;
            edx <<= 8;
            ecx |= edx;
            ecx <<= 8;
            ecx |= eax;
            ecx <<= 8;
            uv_tab_cb_b[i] = static_cast<uint32_t>(ecx);
        }
    } else {
        // 15 / 16-bit layout: the packer 0x00486350 (two pixels per entry).
        int32_t rShift = p->rShift;
        int32_t bBits = p->bBits;
        int32_t gShift = p->gShift;
        int32_t bShift = p->bShift;
        int32_t gBits = p->gBits;
        int32_t rBits = p->rBits;
        for (int32_t i = 0; i < 0x80; i++) {
            int8_t al = static_cast<int8_t>(static_cast<uint8_t>(i) * 2 + 1);
            int32_t v = static_cast<int32_t>(al) + 0x80;
            uv_tab_y[i] = UV_PackColor16(v, v, v, 0xff, rBits, gBits, bBits, rShift, gShift, bShift);
        }
        for (int32_t i = 0; i < 0x80; i++) {
            int8_t cl = static_cast<int8_t>(static_cast<uint8_t>(i) * 2 + 1);
            int32_t c = cl;
            int32_t edi = uv_fmul16(static_cast<int32_t>(0xffffa7ea), c);
            int32_t eax = uv_fmul16(0x1c5a1, c);
            uv_tab_cr_g[i] = UV_PackColor16(0, edi, eax, 0, rBits, gBits, bBits, rShift, gShift, bShift);
        }
        for (int32_t i = 0; i < 0x80; i++) {
            int8_t al = static_cast<int8_t>(static_cast<uint8_t>(i) * 2 + 1);
            int32_t edi = al;
            int32_t ecx = uv_fmul16(0x166e9, edi);
            int32_t eax = uv_fmul16(static_cast<int32_t>(0xffff4931), edi);
            uv_tab_cb_b[i] = UV_PackColor16(ecx, eax, 0, 0, rBits, gBits, bBits, rShift, gShift, bShift);
        }
    }

    // Luma to display value, 1024 steps, two source steps per table entry.
    for (int32_t i = 0; i - 0x180 < 0x280; i++) {
        double d = static_cast<double>(i - 0x180) * static_cast<double>(uv_const_luma);
        int32_t y = static_cast<int32_t>(d);   // __ftol, 0x0048c0d0
        UvSurf *s = o->surf;
        int32_t br = s->brightness;
        int32_t red = uv_div100(static_cast<int32_t>(static_cast<uint32_t>(y) * static_cast<uint32_t>(br)));
        int32_t green = uv_div100(static_cast<int32_t>(static_cast<uint32_t>(y) * static_cast<uint32_t>(br)));
        int32_t blue = uv_div100(static_cast<int32_t>(static_cast<uint32_t>(y) * static_cast<uint32_t>(br)));
        if (red < 0)
            red = 0;
        else if (red > 0xff)
            red = 0xff;
        if (green < 0)
            green = 0;
        else if (green > 0xff)
            green = 0xff;
        if (blue < 0)
            blue = 0;
        else if (blue > 0xff)
            blue = 0xff;
        int32_t idx = i >> 1;
        s = o->surf;
        uv_tab_red[idx] = static_cast<uint32_t>(uv_shl(uv_sar(red, 8 - s->rBits), s->rShift));
        s = o->surf;
        uv_tab_green[idx] = static_cast<uint32_t>(uv_shl(uv_sar(green, 8 - s->gBits), s->gShift));
        s = o->surf;
        uv_tab_blue[idx] = static_cast<uint32_t>(uv_shl(uv_sar(blue, 8 - s->bBits), s->bShift));
        s = o->surf;
        if (s->flags & 0x40) {
            int32_t r2 = uv_shl(1, 7 - s->rBits) + red;
            int32_t g2 = uv_shl(1, 7 - s->gBits) + green;
            int32_t b2 = uv_shl(1, 7 - s->bBits) + blue;
            if (r2 > 0xff)
                r2 = 0xff;
            if (g2 > 0xff)
                g2 = 0xff;
            if (b2 > 0xff)
                b2 = 0xff;
            uv_tab_red[idx] |= static_cast<uint32_t>(uv_shl(uv_sar(r2, 8 - s->rBits), s->rShift + 0x10));
            s = o->surf;
            uv_tab_green[idx] |= static_cast<uint32_t>(uv_shl(uv_sar(g2, 8 - s->gBits), s->gShift + 0x10));
            s = o->surf;
            uv_tab_blue[idx] |= static_cast<uint32_t>(uv_shl(uv_sar(b2, 8 - s->bBits), s->bShift + 0x10));
        }
    }

    // Chroma contributions, one entry per signed byte.
    for (int32_t v = -128; v < 0x80; v++) {
        double dv = static_cast<double>(v);
        uint32_t slot = static_cast<uint32_t>(v) & 0xff;
        int32_t a = static_cast<int32_t>(dv * static_cast<double>(uv_const_cr_r) * uv_const_k);   // __ftol
        int32_t b = static_cast<int32_t>(dv * static_cast<double>(uv_const_cb_b) * uv_const_k);   // __ftol
        uv_tab_cr_r[slot] = static_cast<uint32_t>(a) << 2;
        int32_t c = static_cast<int32_t>(dv * static_cast<double>(uv_const_cb_g) * uv_const_k);   // __ftol
        uv_tab_cb_b32[slot] = static_cast<uint32_t>(b) << 2;
        int32_t d = static_cast<int32_t>(dv * static_cast<double>(uv_const_cr_g) * uv_const_k);   // __ftol
        uv_tab_cb_g[slot] = static_cast<uint32_t>(c) << 2;
        uv_tab_cr_gptr[slot] = GADDR(dat_0075e408) - (static_cast<uint32_t>(-d) << 2);
    }
}
HOOK_FUNC(0x00485da0, UV_BuildTables);
