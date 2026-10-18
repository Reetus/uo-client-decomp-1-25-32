// uV intro video player, DCT codec: the block-level pieces (batch v3, hand-written assembly).
//   0x00486e77  sparse coefficient decoder        0x00487017  VLC coefficient decoder
//   0x0048732c  row colour output, 32-bit pixels  0x00487557  row colour output, 16-bit pixels
//   0x00488bed  row output through pointer LUTs (32-bit)       0x00488ec0  same, two pixels packed per dword
//   0x00489c42  IDCT pass 1                       0x00489e6f  IDCT pass 2
// None of these has a caller outside the codec, so none gets an entry hook: the macroblock drivers
// (uv_dct_mb.cpp) call the C++ directly. Their register conventions, for the record:
//   0x00486e77: no arguments, state in UVA_DATA (0x0075f120/4/8). Clobbers every register.
//   0x00487017: EAX = DC value, EBP = stream base, EDI = bit position; stores EDI to 0x0075f124 on exit.
//   0x0048732c / 0x00487557: ESI = luma row (16 dwords), EBX = chroma LUT row, EDI = destination.
//   0x00488bed / 0x00488ec0: ESI = luma row, EDI = LUT row (kept), EBP = destination.
//   0x00489c42 / 0x00489e6f: ESI = 8 input dwords, [esp+4] = output; scratch is the caller's frame [esp+4..0x13].
#include "uv_dct.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"

namespace {

// Dword read at an arbitrary 32-bit address (the colour tables hold pointers into other tables).
inline uint32_t Rd(uint32_t addr)
{
    return *reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(addr));
}

inline uint32_t Shl(uint32_t v, uint32_t cl)
{
    return v << (cl & 31);
}

// imul r/m32 followed by "shl edx,16; shr eax,16; adc eax,edx": the 16.16 product rounded by the bit shifted out.
inline uint32_t MulRound(int32_t a, int32_t b)
{
    int64_t p = static_cast<int64_t>(a) * b;
    return static_cast<uint32_t>(p >> 16) + static_cast<uint32_t>((p >> 15) & 1);
}

// 32 bits of the stream starting at bit `pos` (mov/mov/shld).
inline uint32_t Peek(const uint32_t *base, uint32_t pos)
{
    uint32_t w0 = base[pos >> 5];
    uint32_t w1 = base[(pos >> 5) + 1];
    return static_cast<uint32_t>(((static_cast<uint64_t>(w0) << 32) | w1) << (pos & 31) >> 32);
}

} // namespace

// 0x00486e77  size 0x1a0 (+ the 0x144-byte case blob at 0x00486ed3)
// Sparse coefficient decoder. The first byte of the buffer is the DC (signed, scaled by the first quantiser); then
// for scan positions 63..1 a 3-bit tag chooses: 0 = zero, 2 / 6 = +-1, 4 / 5 / 1 = runs of zeros, 3 / 7 = a 6-bit
// value (all-ones escapes to a following byte). State: 0x75f120 byte cursor, 0x75f128 bit buffer, 0x75f124 the
// number of valid bits. The jump table at 0x00760c40 is dispatched as a switch.
void uv_dct_decode_block_sparse()
{
    uint32_t ebp = UVD_BitBuf;
    const uint8_t *esi = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    uint32_t ecx = UVD_BitPos;
    int32_t *coeff = UVD_Coeff;
    const int32_t *quant = UVD_Quant;
    const uint32_t *scan = UVD_Scan;

    uint32_t eax = static_cast<uint32_t>(static_cast<int32_t>(UVD_BitBuf << 24) >> 8);
    coeff[0] = static_cast<int32_t>(MulRound(static_cast<int32_t>(eax), quant[0]));
    int32_t edi = 0x3f;
    uint32_t ebx = 0;

    ebp >>= 8;
    eax = *esi++;
    ecx -= 8;
    eax = Shl(eax, ecx);
    ecx += 8;
    ebp |= eax;

    for (;;) { // 0x00486ec0
        ebx = scan[edi];
        bool wide = false; // true: tag consumed 8 bits (0x00486feb), false: 3 bits (0x00486f20)
        switch (ebp & 7) {
        case 2:
            coeff[ebx] = quant[ebx];
            break;
        case 6:
            coeff[ebx] = static_cast<int32_t>(0u - static_cast<uint32_t>(quant[ebx]));
            break;
        case 0:
            coeff[ebx] = 0;
            break;
        case 4:
            coeff[ebx] = 0;
            ebx = scan[edi - 1];
            edi--;
            coeff[ebx] = 0;
            break;
        case 5:
            coeff[ebx] = 0;
            ebx = scan[edi - 1];
            edi--;
            /* falls through into case 1 */
        case 1: {
            uint32_t count = (ebp & 0xff) >> 3;
            do {
                coeff[ebx] = 0;
                ebx = scan[edi - 1];
                coeff[ebx] = 0;
                ebx = scan[edi - 2];
                edi -= 2;
                count--;
            } while (count != 0);
            edi++;
            wide = true;
            break;
        }
        default: { // 3, 7
            eax = ebp & 0xfc;
            if (eax == 0xfc) {
                ebp >>= 8;
                eax = *esi++;
                ecx -= 8;
                eax = Shl(eax, ecx);
                ecx += 8;
                ebp |= eax;
                eax = static_cast<uint32_t>(static_cast<int32_t>(ebp << 24) >> 8);
                coeff[ebx] = static_cast<int32_t>(MulRound(static_cast<int32_t>(eax), quant[ebx]));
            } else {
                eax = static_cast<uint32_t>(static_cast<int32_t>(eax << 24) >> 10);
                coeff[ebx] = static_cast<int32_t>(MulRound(static_cast<int32_t>(eax), quant[ebx]));
            }
            wide = true;
            break;
        }
        }
        if (!wide) {
            ebp >>= 3;
            ecx -= 3;
            if (static_cast<int32_t>(ecx) < 8) {
                eax = *esi++;
                eax = Shl(eax, ecx);
                ecx += 8;
                ebp |= eax;
            }
        } else {
            ebp >>= 8;
            eax = *esi++;
            ecx -= 8;
            eax = Shl(eax, ecx);
            ecx += 8;
            ebp |= eax;
        }
        edi--;
        if (edi <= 0)
            break;
    }
    UVD_BitPtr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(esi));
    UVD_BitBuf = ebp;
    UVD_BitPos = ecx;
}
PORT_NOHOOK_FUNC(0x00486e77, uv_dct_decode_block_sparse);

// 0x00487017  size 0x315
// VLC coefficient decoder (MSB-first stream read through dword windows). EAX = DC value, EBP = stream base,
// EDI = bit position. Clears the block, stores dc * quant[0], then decodes (run, level, sign) codes until the
// end-of-block code; escape codes carry a 6-bit run and an 8/16-bit level. The final bit position goes to 0x75f124.
void uv_dct_decode_block_vlc(int32_t dc, const uint32_t *base, uint32_t pos)
{
    int32_t *coeff = UVD_Coeff;
    const int32_t *quant = UVD_Quant;
    const uint32_t *scan = UVD_Scan;

    coeff[0] = static_cast<int32_t>(static_cast<uint32_t>(dc) * static_cast<uint32_t>(quant[0]));
    for (int i = 1; i < 64; i++)
        coeff[i] = 0;
    uint32_t ebx = 0x3f;
    uint32_t edi = pos;

    for (;;) { // 0x004871a3
        uint32_t eax = Peek(base, edi) >> 15;
        uint32_t ecx;
        uint32_t entry;
        if (static_cast<int32_t>(eax) >= 0x8000) {
            ecx = eax;
            eax >>= 13;
            entry = Rd(GADDR(dat_00760830) + eax * 4);
        } else {
            ecx = eax;
            eax >>= 1;
            if (static_cast<int32_t>(eax) >= 0x400) {
                eax >>= 8;
                entry = Rd(GADDR(dat_00760860) + eax * 4);
            } else if (static_cast<int32_t>(eax) >= 0x200) {
                eax >>= 6;
                entry = Rd(GADDR(dat_00760940) + eax * 4);
            } else if (static_cast<int32_t>(eax) >= 0x100) {
                eax >>= 4;
                entry = Rd(GADDR(dat_00760940) + eax * 4);
            } else if (static_cast<int32_t>(eax) >= 0x80) {
                eax >>= 3;
                entry = Rd(GADDR(dat_00760980) + eax * 4);
            } else if (static_cast<int32_t>(eax) >= 0x40) {
                eax >>= 2;
                entry = Rd(GADDR(dat_007609c0) + eax * 4);
            } else if (static_cast<int32_t>(eax) >= 0x20) {
                eax >>= 1;
                entry = Rd(GADDR(dat_00760a00) + eax * 4);
            } else if (static_cast<int32_t>(eax) >= 0x10) {
                entry = Rd(GADDR(dat_00760a40) + eax * 4);
            } else {
                UVD_BitPos = edi; // 0x0048720a
                return;
            }
        }
        // 0x004871cb: entry = len << 16 | run << 8 | level
        int32_t len = static_cast<int32_t>(entry) >> 16;
        edi += static_cast<uint32_t>(len);
        uint32_t run = (entry >> 8) & 0xff;
        if (run == 0x41) {
            UVD_BitPos = edi;
            return;
        }
        if (run > 0x41) {
            // escape: 6-bit run, then an 8-bit level (0 and 0x80 mean a following byte)
            eax = Peek(base, edi) >> 18;
            edi += 14;
            ebx -= eax >> 8;
            eax &= 0xff;
            if (eax == 0) {
                eax = Peek(base, edi) >> 24;
                edi += 8;
            } else if (eax == 0x80) {
                eax = Peek(base, edi) >> 24;
                edi += 8;
                eax -= 0x100;
            } else if (static_cast<int32_t>(eax) > 0x80) {
                eax -= 0x100;
            }
            uint32_t idx = scan[ebx];
            coeff[idx] = static_cast<int32_t>(eax * static_cast<uint32_t>(quant[idx]));
            ebx--;
            continue;
        }
        ebx = (ebx & ~0xffu) | ((ebx - run) & 0xff); // sub bl, dh
        uint32_t level = entry & 0xff;
        uint32_t sh = (0u - static_cast<uint32_t>(len)) + 0x10;
        edi++;
        if ((ecx >> (sh & 31)) & 1)
            level = 0u - level;
        uint32_t idx = scan[ebx + 1];
        coeff[idx] = static_cast<int32_t>(level * static_cast<uint32_t>(quant[idx]));
    }
}
PORT_NOHOOK_FUNC(0x00487017, uv_dct_decode_block_vlc);

// 0x0048732c  size 0x22b
// One output row of 16 pixels, 32 bits each: dst[2i], dst[2i+1] = lumaLUT[luma & 0x7f] + cbLUT[cb[i] & 0x7f] +
// crLUT[cr[i] & 0x7f]. ESI = 16 luma dwords, EBX = 8 Cb dwords (Cr is 0x100 bytes after), EDI = destination.
void uv_dct_output_block_s32(const int32_t *luma, const uint32_t *chroma, uint32_t *dst)
{
    for (int i = 0; i < 8; i++) {
        uint32_t a = Rd(GADDR(uv_tab_y) + (static_cast<uint32_t>(luma[2 * i]) & 0x7f) * 4);
        uint32_t b = Rd(GADDR(uv_tab_y) + (static_cast<uint32_t>(luma[2 * i + 1]) & 0x7f) * 4);
        uint32_t cb = Rd(GADDR(uv_tab_cr_g) + (chroma[i] & 0x7f) * 4);
        uint32_t cr = Rd(GADDR(uv_tab_cb_b) + (chroma[0x40 + i] & 0x7f) * 4);
        a += cb;
        b += cb;
        a += cr;
        b += cr;
        dst[2 * i] = a;
        dst[2 * i + 1] = b;
    }
}
PORT_NOHOOK_FUNC(0x0048732c, uv_dct_output_block_s32);

// 0x00487557  size 0x23b
// Same as 0x0048732c but each pixel is stored as a 16-bit word (low half of the sum).
void uv_dct_output_block_s16(const int32_t *luma, const uint32_t *chroma, uint16_t *dst)
{
    for (int i = 0; i < 8; i++) {
        uint32_t a = Rd(GADDR(uv_tab_y) + (static_cast<uint32_t>(luma[2 * i]) & 0x7f) * 4);
        uint32_t b = Rd(GADDR(uv_tab_y) + (static_cast<uint32_t>(luma[2 * i + 1]) & 0x7f) * 4);
        uint32_t cb = Rd(GADDR(uv_tab_cr_g) + (chroma[i] & 0x7f) * 4);
        uint32_t cr = Rd(GADDR(uv_tab_cb_b) + (chroma[0x40 + i] & 0x7f) * 4);
        a += cb;
        b += cb;
        a += cr;
        b += cr;
        dst[2 * i] = static_cast<uint16_t>(a);
        dst[2 * i + 1] = static_cast<uint16_t>(b);
    }
}
PORT_NOHOOK_FUNC(0x00487557, uv_dct_output_block_s16);

// The per-row pointer arithmetic shared by 0x00488bed and 0x00488ec0: the two chroma values select (through the
// tables at 0x75f72c / 0x75ff2c and 0x75fb2c / 0x76032c) three bases, and a luma value indexes all three.
namespace {
struct LutRow {
    uint32_t e1, cc, dd;
    explicit LutRow(const uint32_t *lut, int i)
    {
        uint32_t c = lut[0x40 + i];
        uint32_t d = lut[i];
        e1 = Rd(GADDR(uv_tab_cr_gptr) + c * 4) - Rd(GADDR(uv_tab_cb_g) + d * 4);
        cc = Rd(GADDR(uv_tab_cr_r) + c * 4) + GADDR(dat_0075d9a0);
        dd = Rd(GADDR(uv_tab_cb_b32) + d * 4) + GADDR(dat_0075e944);
    }
    uint32_t At(int32_t a) const
    {
        uint32_t o = static_cast<uint32_t>(a) * 4u;
        return Rd(cc + o) + Rd(dd + o) + Rd(e1 + o);
    }
};
} // namespace

// 0x00488bed  size 0x2d3
// ESI = 16 luma dwords, EDI = 8 + 8 LUT selector dwords (kept), EBP = destination of 16 dwords.
void uv_dct_output_block_e(const int32_t *luma, const uint32_t *lut, uint32_t *dst)
{
    for (int i = 0; i < 8; i++) {
        LutRow r(lut, i);
        dst[2 * i + 1] = r.At(luma[2 * i + 1]);
        dst[2 * i] = r.At(luma[2 * i]);
    }
}
PORT_NOHOOK_FUNC(0x00488bed, uv_dct_output_block_e);

// 0x00488ec0  size 0x2d3
// As 0x00488bed, but the two pixels of a pair are packed into one dword: (second << 16) + first. 8 dwords out.
void uv_dct_output_block_f(const int32_t *luma, const uint32_t *lut, uint32_t *dst)
{
    for (int i = 0; i < 8; i++) {
        LutRow r(lut, i);
        uint32_t second = r.At(luma[2 * i + 1]);
        uint32_t first = r.At(luma[2 * i]);
        dst[i] = (second << 16) + first;
    }
}
PORT_NOHOOK_FUNC(0x00488ec0, uv_dct_output_block_f);

namespace {
// The 8-point transform both passes share (the two procedures differ only in where they store the result and in
// the final shift). x87 work is done in double: fild, fmul by a float constant, fadd, and the 1.5 * 2^52 magic
// add that turns a double into the integer in its low mantissa dword.
void Idct8(const int32_t *in, uint32_t r[8])
{
    uint32_t in0 = static_cast<uint32_t>(in[0]), in1 = static_cast<uint32_t>(in[1]);
    uint32_t in2 = static_cast<uint32_t>(in[2]), in3 = static_cast<uint32_t>(in[3]);
    uint32_t in4 = static_cast<uint32_t>(in[4]), in5 = static_cast<uint32_t>(in[5]);
    uint32_t in6 = static_cast<uint32_t>(in[6]), in7 = static_cast<uint32_t>(in[7]);

    uint32_t s35 = in3 + in5;
    uint32_t d53 = in5 - in3;
    uint32_t s71 = in7 + in1;
    uint32_t d17 = in1 - in7;
    uint32_t sum = s35 + s71;
    uint32_t dif = s71 - s35;

    const double c0 = uv_00760730;
    const double c1 = uv_00760734;
    const double c2 = uv_00760738;
    const double magic = uv_0076073c;
    const int32_t k = uv_0076072c;

    double a = static_cast<double>(static_cast<int32_t>(d53));
    double b = static_cast<double>(static_cast<int32_t>(d17));
    double t1 = b * c1;
    double ab = b + a;
    double t0 = a * c0;
    double t2 = ab * c2;
    double p = t0 + t2;
    double q = t1 - t2;
    double pm = p + magic;
    double qm = q + magic;
    int64_t prod = static_cast<int64_t>(static_cast<int32_t>(dif)) * k;
    uint32_t h = static_cast<uint32_t>(prod >> 32) << 1;
    uint64_t pbits, qbits;
    std::memcpy(&pbits, &pm, 8);
    std::memcpy(&qbits, &qm, 8);
    uint32_t pi = static_cast<uint32_t>(pbits);
    uint32_t qi = static_cast<uint32_t>(qbits);
    uint32_t x1 = pi + h;
    uint32_t x2 = h + qi;
    uint32_t x3 = qi + sum;

    uint32_t g0 = in6 + in2;
    uint32_t e = in2 - in6;
    uint32_t z0 = in4 + in0;
    uint32_t z1 = in0 - in4;
    int64_t prod2 = static_cast<int64_t>(static_cast<int32_t>(e)) * k;
    uint32_t h2 = static_cast<uint32_t>(prod2 >> 32) << 1;
    uint32_t g = g0 + h2;
    uint32_t l1 = h2 + z1;
    uint32_t l2 = z1 - h2;
    uint32_t t0i = g + z0;
    uint32_t t3i = z0 - g;

    r[2] = l2 + x1;
    r[5] = l2 - x1;
    r[3] = t3i + pi;
    r[4] = t3i - pi;
    r[1] = x2 + l1;
    r[6] = l1 - x2;
    r[0] = t0i + x3;
    r[7] = t0i - x3;
}
} // namespace

// 0x00489c42  size 0x141
// IDCT pass 1 for one input row (ESI, 8 dwords). The output goes to [esp+4] with element k at out[9 * k]
// (a 9-dword pitch, the transposed intermediate that pass 2 reads). A row whose elements 1..7 are all zero is
// flat: element 0 is copied to every output. The scratch the original keeps in the caller's frame is local here.
void uv_dct_idct_pass1(const int32_t *in, int32_t *out)
{
    if ((in[1] | in[2] | in[3] | in[4] | in[5] | in[6] | in[7]) == 0) {
        for (int k = 0; k < 8; k++)
            out[9 * k] = in[0];
        return;
    }
    uint32_t r[8];
    Idct8(in, r);
    for (int k = 0; k < 8; k++)
        out[9 * k] = static_cast<int32_t>(r[k]);
}
PORT_NOHOOK_FUNC(0x00489c42, uv_dct_idct_pass1);

// 0x00489e6f  size 0x102
// IDCT pass 2: the same transform without the flat-row shortcut, every result arithmetically shifted right by 17,
// stored contiguously at out[k].
void uv_dct_idct_pass2(const int32_t *in, int32_t *out)
{
    uint32_t r[8];
    Idct8(in, r);
    for (int k = 0; k < 8; k++)
        out[k] = static_cast<int32_t>(r[k]) >> 17;
}
PORT_NOHOOK_FUNC(0x00489e6f, uv_dct_idct_pass2);
