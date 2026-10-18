// uV intro video player, 8-bit indexed-frame codec: frame setup, buffer allocation, colour table,
// frame blit, VGA palette, the EA RefPack decompressor, bitfield unpacking and row copies.
// (Porting pass 3, batch v4: 0x00489c0a - 0x0048aacb.)
//
// Much of this is hand-written assembly. Entries called from the frame decoder 0x00486850:
//   0x0048a3f0 uv_alloc_frame_buffers : ECX = player, EDX = stream pointer; returns EAX = 1 / 0.
//   0x0048a550 uv_build_color_table   : ECX = player; nothing returned.
//   0x00489f80 uv_decode_frame_setup  : ECX = player, EDX = stream pointer; returns EAX = 1 / 0.
//   0x0048a610 uv_blit_frame          : ECX = player, EDX = destination, one unread stack dword, `ret 4`.
// All four fit __fastcall (ECX / EDX plus callee-popped stack) and preserve EBX / ESI / EDI / EBP as the C++
// callee-saved set does; the originals clobber only EAX, ECX, EDX. The remaining procedures are only called from
// inside this file (the whole image scanned for call / jmp / data references) and are plain __cdecl C++.
// The zeroed 0x6c-byte local block of uv_blit_frame is never read and is left out.
#include "uv_frame8.h"

#include <intrin.h>

#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"

// CRT malloc (not operator new): reads the heap handle at 0x00758b38.

static uint32_t Uv8Rd16(const uint8_t *p)
{
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}

static uint32_t Uv8Rd32(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

// x86 shifts use the low five bits of the count.
static uint32_t Uv8Shr(uint32_t v, uint32_t n) { return v >> (n & 31); }
static uint32_t Uv8Shl(uint32_t v, uint32_t n) { return v << (n & 31); }
static uint32_t Uv8Ror(uint32_t v, uint32_t n)
{
    n &= 31;
    return n == 0 ? v : (v >> n) | (v << (32 - n));
}

// 0x00489c0a  size 0x38  cdecl(src, dst, count): every source byte becomes a 16-bit pair of itself; two bytes per
// dword written, count is consumed two at a time.
static void UvExpandBytes(const uint8_t *src, uint32_t *dst, uint32_t count)
{
    do {
        uint32_t lo = src[0];
        lo |= lo << 8;
        uint32_t hi = src[1];
        hi |= hi << 8;
        src += 2;
        *dst = lo | (hi << 16);
        dst++;
        count -= 2;
    } while (count != 0);
}
HOOK_FUNC(0x00489c0a, UvExpandBytes);

// 0x0048a96f  size 0xa9  cdecl(dst, src, count, bits): unpacks `count` fields of `bits` bits each from the
// little-endian bitstream into dwords. Each field is read through a dword rotated right (shrd eax, eax, cl).
static void UvUnpackBitfields(uint32_t *dst, const uint8_t *src, int32_t count, uint32_t bits)
{
    uint32_t bitpos = 0;
    int32_t left = count;
    uint32_t mask = Uv8Shl(1, bits) - 1;
    left -= 4;
    if (left >= 0) {
        for (;;) {
            uint32_t v = Uv8Rd32(src + (bitpos >> 3));
            v = Uv8Ror(v, bitpos & 7);
            dst[0] = v & mask;
            v = Uv8Ror(v, bits);
            dst[1] = v & mask;
            uint32_t p = bitpos + bits * 2;
            v = Uv8Rd32(src + (p >> 3));
            v = Uv8Ror(v, p & 7);
            dst[2] = v & mask;
            v = Uv8Ror(v, bits);
            dst[3] = v & mask;
            dst += 4;
            left -= 4;
            bitpos += bits * 4;
            if (left < 0) {
                break;
            }
        }
    }
    left += 3;
    if (left >= 0) {
        do {
            uint32_t v = Uv8Rd32(src + (bitpos >> 3));
            v = Uv8Ror(v, bitpos & 7);
            *dst = v & mask;
            bitpos += bits;
            left -= 1;
            dst++;
        } while (left >= 0);
    }
}
HOOK_FUNC(0x0048a96f, UvUnpackBitfields);

// 0x0048aa18  size 0xb3 (+ the `ret` at 0x0048aacb)  cdecl of nine dwords. Rebuilds the frame in 4x4 pixel blocks
// (one dword per row of a block): each entry of `indices` below `splitIndex` copies the block at the motion offset
// offsets[index] from the previous frame, any other entry takes 16 bytes from the block table.
static void UvCopyRows(const int32_t *indices, uint8_t *oldFrame, uint8_t *newFrame, int32_t splitIndex,
                       const int32_t *offsets, const uint8_t *blockTable, uint32_t stride, int32_t blocksPerRow,
                       int32_t blockRows)
{
    int32_t perRowLeft;
    uintptr_t tableBase = reinterpret_cast<uintptr_t>(blockTable) - (static_cast<uint32_t>(splitIndex) << 4);
    uintptr_t frameDelta = reinterpret_cast<uintptr_t>(oldFrame) - reinterpret_cast<uintptr_t>(newFrame);
    uint32_t stride3 = stride * 3;
    const int32_t *idx = indices;
    uintptr_t dst = reinterpret_cast<uintptr_t>(newFrame);
    do {
        perRowLeft = blocksPerRow;
        int32_t e = *idx;
        idx++;
        for (;;) {
            if (e < splitIndex) {
                // Motion block: 4 dwords from the old frame at offsets[e] + (dst - newFrame).
                for (;;) {
                    uintptr_t src = static_cast<uintptr_t>(static_cast<uint32_t>(offsets[e])) + frameDelta + dst;
                    *reinterpret_cast<uint32_t *>(dst) = *reinterpret_cast<const uint32_t *>(src);
                    *reinterpret_cast<uint32_t *>(dst + stride) = *reinterpret_cast<const uint32_t *>(src + stride);
                    *reinterpret_cast<uint32_t *>(dst + stride * 2) = *reinterpret_cast<const uint32_t *>(src + stride * 2);
                    *reinterpret_cast<uint32_t *>(dst + stride3) = *reinterpret_cast<const uint32_t *>(src + stride3);
                    dst += 4;
                    perRowLeft -= 1;
                    if (perRowLeft <= 0) {
                        goto rowEnd;
                    }
                    e = *idx;
                    idx++;
                    if (!(e < splitIndex)) {
                        break;
                    }
                }
            }
            {
                // Table block: 16 bytes at tableBase + e * 16.
                const uint8_t *blk = reinterpret_cast<const uint8_t *>(tableBase + (static_cast<uint32_t>(e) << 4));
                *reinterpret_cast<uint32_t *>(dst) = Uv8Rd32(blk);
                *reinterpret_cast<uint32_t *>(dst + stride) = Uv8Rd32(blk + 4);
                *reinterpret_cast<uint32_t *>(dst + stride * 2) = Uv8Rd32(blk + 8);
                *reinterpret_cast<uint32_t *>(dst + stride3) = Uv8Rd32(blk + 12);
                dst += 4;
                perRowLeft -= 1;
                if (perRowLeft <= 0) {
                    break;
                }
                e = *idx;
                idx++;
            }
        }
    rowEnd:
        dst += stride3;
        blockRows -= 1;
    } while (blockRows > 0);
}
HOOK_FUNC(0x0048aa18, UvCopyRows);

// 0x0048a84c  size 0x11d (+ the `jmp 0x0048a8c4` at 0x0048a96a)  cdecl(src, dst, flag): EA RefPack decompressor.
// Header: 2 flag bytes (bit 0 of the first = a 3-byte compressed size follows), then the 24-bit big-endian
// unpacked size. Returns that size; a zero `flag` stops after the header and a null `src` returns 0.
// Commands (first byte): 0xxxxxxx 2 bytes, 10xxxxxx 3 bytes, 110xxxxx 4 bytes, 111xxxxx literal run / stop (>= 0xfc).
// The back-reference copy is a forward byte copy, so overlapping runs repeat.
static uint32_t UvRefPackDecompress(const uint8_t *src, uint8_t *dst, uint32_t flag)
{
    if (src == nullptr) {
        return 0;
    }
    const uint8_t *s = src;
    uint32_t head = Uv8Rd16(s);
    s += 2;
    if (head & 1) {
        s += 3;
    }
    uint32_t size = (static_cast<uint32_t>(s[0]) << 16) | (static_cast<uint32_t>(s[1]) << 8) | s[2];
    s += 3;
    if (flag == 0) {
        return size;
    }
    uint8_t *d = dst;
    for (;;) {
        uint32_t cmd = Uv8Rd32(s);
        uint32_t b0 = cmd & 0xff;
        uint32_t b1 = (cmd >> 8) & 0xff;
        uint32_t b2 = (cmd >> 16) & 0xff;
        uint32_t b3 = cmd >> 24;
        uint32_t lit;
        uint32_t offset;
        uint32_t run;
        if (!(b0 & 0x80)) {
            lit = b0 & 3;
            for (uint32_t i = 0; i < lit; i++) {
                d[i] = s[2 + i];
            }
            d += lit;
            s += 2 + lit;
            offset = ((b0 >> 5) << 8) | b1;
            run = ((b0 & 0x1c) >> 2) + 3;
        } else if (!(b0 & 0x40)) {
            lit = b1 >> 6;
            for (uint32_t i = 0; i < lit; i++) {
                d[i] = s[3 + i];
            }
            d += lit;
            s += 3 + lit;
            offset = ((b1 & 0x3f) << 8) | b2;
            run = (b0 & 0x3f) + 4;
        } else if (!(b0 & 0x20)) {
            lit = b0 & 3;
            for (uint32_t i = 0; i < lit; i++) {
                d[i] = s[4 + i];
            }
            d += lit;
            s += 4 + lit;
            offset = ((b0 & 0x10) << 12) | (b1 << 8) | b2;
            run = ((((b0 >> 2) & 3) << 8) | b3) + 5;
        } else if (b0 >= 0xfc) {
            lit = b0 & 3;
            for (uint32_t i = 0; i < lit; i++) {
                d[i] = s[1 + i];
            }
            return size;
        } else {
            uint32_t words = (b0 & 0x1f) + 1;
            memcpy(d, s + 1, words * 4);
            d += words * 4;
            s += 1 + words * 4;
            continue;
        }
        const uint8_t *from = d - 1 - offset;
        for (uint32_t i = 0; i < run; i++) {
            d[i] = from[i];
        }
        d += run;
    }
}
HOOK_FUNC(0x0048a84c, UvRefPackDecompress);

// 0x0048a810  size 0x3c  cdecl(palette): programs the VGA DAC through ports 0x3c8 / 0x3c9, 256 entries of
// 4 bytes, 8 bits scaled down to 6. Privileged instructions: faults on NT exactly as the original (dead code).
static void UvSetVgaPalette(const uint8_t *pal)
{
    _disable();
    __outbyte(0x3c8, 0);
    uint32_t count = 0x100;
    do {
        __outbyte(0x3c9, static_cast<uint8_t>(pal[0] >> 2));
        __outbyte(0x3c9, static_cast<uint8_t>(pal[1] >> 2));
        __outbyte(0x3c9, static_cast<uint8_t>(pal[2] >> 2));
        pal += 4;
        count--;
    } while (count != 0);
    _enable();
}
HOOK_FUNC(0x0048a810, UvSetVgaPalette);

// 0x0048a3f0  size 0x15a  ECX = player, EDX = stream pointer (the RefPack data starts 3 * (extra + 4) bytes in).
// Frees and reallocates the two frame buffers, the row-offset table and the index buffer, fills the row-offset
// table with -(w*h) + k*w and decompresses the stream into the first frame. Returns 1, or 0 when an allocation
// failed (the buffers stay allocated).
int __fastcall UvAllocFrameBuffers(UvPlayer8 *self, const uint8_t *stream)
{
    int32_t extra = self->streamExtra;
    uint32_t ebx = 0;
    uint32_t size = self->fmt->height * self->fmt->width;
    size = (size + 0xf) & 0xfffffff0u;
    if (self->frameCur != nullptr) {
        free(self->frameCur);
        self->frameCur = nullptr;
    }
    if (self->frameOld != nullptr) {
        free(self->frameOld);
        self->frameOld = nullptr;
    }
    if (self->rowOffsets != nullptr) {
        free(self->rowOffsets);
        self->rowOffsets = nullptr;
    }
    if (self->indexArray != nullptr) {
        free(self->indexArray);
        self->indexArray = nullptr;
    }
    self->frameCur = static_cast<uint8_t *>(malloc(size));
    self->frameOld = static_cast<uint8_t *>(malloc(size));
    self->rowOffsets = static_cast<int32_t *>(malloc(self->fmt->height << 3));
    int32_t blocks = static_cast<int32_t>(size);
    blocks += (blocks >> 31) & 0xf; // cdq; and edx, 0xf; add
    blocks >>= 4;
    self->indexArray = static_cast<int32_t *>(malloc(static_cast<uint32_t>(blocks) << 2));
    if (self->frameCur != nullptr && self->frameOld != nullptr && self->rowOffsets != nullptr &&
        self->indexArray != nullptr) {
        uint32_t width = self->fmt->width;
        uint32_t value = 0u - width * self->fmt->height;
        uint32_t i = 0;
        if (self->fmt->height * 2 != 0) {
            do {
                self->rowOffsets[i] = static_cast<int32_t>(value);
                value += self->fmt->width;
                i++;
            } while (i < self->fmt->height * 2);
        }
        int32_t base = extra + 4;
        UvRefPackDecompress(stream + (base + 2 * base), self->frameCur, 1);
        ebx = 1;
    }
    return static_cast<int>(ebx);
}
HOOK_FUNC(0x0048a3f0, UvAllocFrameBuffers);

// 0x0048a550  size 0xb3  ECX = player. Builds the 256-entry colour table from the 3-byte palette: each byte is
// cut to the channel's bit count, shifted into place and the three ORed; with flag 0x40 the pixel is doubled
// into both halves of the dword.
void __fastcall UvBuildColorTable(UvPlayer8 *self)
{
    UvOutFormat8 *f = self->fmt;
    uint32_t redCut = 8 - f->redBits;
    uint32_t greenCut = 8 - f->greenBits;
    uint32_t blueCut = 8 - f->blueBits;
    uint32_t *out = self->colorTable;
    const uint8_t *p = self->palette + 1;
    uint32_t left = 0x100;
    do {
        f = self->fmt;
        uint32_t v = Uv8Shl(Uv8Shr(p[0], greenCut), f->greenShift);
        uint32_t t = Uv8Shl(Uv8Shr(p[1], blueCut), f->blueShift);
        v |= t;
        t = Uv8Shl(Uv8Shr(p[-1], redCut), f->redShift);
        v |= t;
        if (f->flags & 0x40) {
            v |= v << 16;
        }
        *out = v;
        out++;
        p += 3;
        left--;
    } while (left != 0);
}
HOOK_FUNC(0x0048a550, UvBuildColorTable);

// 0x0048a610  size 0x1fc  ECX = player, EDX = destination, one unread stack dword (ret 4). Copies the decoded
// 8-bit frame to the destination: bytes as they are, expanded to 16 bit pairs, or through the colour table to
// 32-bit (flag 0x40) or 16-bit pixels, eight pixels per inner iteration.
void __fastcall UvBlitFrame(UvPlayer8 *self, uint8_t *dst, uint32_t)
{
    UvOutFormat8 *f = self->fmt;
    const uint8_t *src = self->frameCur;
    if (f->bitsPerPixel == 8) {
        if (f->flags & 0x40) {
            UvExpandBytes(src, reinterpret_cast<uint32_t *>(dst), f->width * f->height);
            return;
        }
        memcpy(dst, src, f->width * f->height);
        return;
    }
    if (f->flags & 0x40) {
        uint32_t *out = reinterpret_cast<uint32_t *>(dst);
        uint32_t y = 0;
        if (f->height != 0) {
            do {
                uint32_t x = 0;
                if (f->width != 0) {
                    do {
                        x += 8;
                        out[0] = self->colorTable[src[0]];
                        out[1] = self->colorTable[src[1]];
                        out[2] = self->colorTable[src[2]];
                        out[3] = self->colorTable[src[3]];
                        out[4] = self->colorTable[src[4]];
                        out[5] = self->colorTable[src[5]];
                        out[6] = self->colorTable[src[6]];
                        out[7] = self->colorTable[src[7]];
                        src += 8;
                        out += 8;
                    } while (x < self->fmt->width);
                }
                f = self->fmt;
                y++;
            } while (y < f->height);
        }
        return;
    }
    uint16_t *out = reinterpret_cast<uint16_t *>(dst);
    uint32_t y = 0;
    if (f->height != 0) {
        do {
            uint32_t x = 0;
            if (f->width != 0) {
                do {
                    x += 8;
                    out[0] = static_cast<uint16_t>(self->colorTable[src[0]]);
                    out[1] = static_cast<uint16_t>(self->colorTable[src[1]]);
                    out[2] = static_cast<uint16_t>(self->colorTable[src[2]]);
                    out[3] = static_cast<uint16_t>(self->colorTable[src[3]]);
                    out[4] = static_cast<uint16_t>(self->colorTable[src[4]]);
                    out[5] = static_cast<uint16_t>(self->colorTable[src[5]]);
                    out[6] = static_cast<uint16_t>(self->colorTable[src[6]]);
                    out[7] = static_cast<uint16_t>(self->colorTable[src[7]]);
                    src += 8;
                    out += 8;
                } while (x < self->fmt->width);
            }
            f = self->fmt;
            y++;
        } while (y < f->height);
    }
}
HOOK_FUNC(0x0048a610, UvBlitFrame);

// 0x00489f80  size 0x466  ECX = player, EDX = frame stream. Swaps the two frame buffers, reads four u16 counts
// (motion entries, raw blocks, 2-bit-index blocks, index bit width), (re)allocates the pair / offset / block
// tables, decodes the 10-bit signed motion pairs (stored backwards) into offsets, expands the block table,
// unpacks the index bitfield and rebuilds the frame with UvCopyRows. Returns 1, or 0 when an allocation failed.
int __fastcall UvDecodeFrameSetup(UvPlayer8 *self, const uint8_t *stream)
{
    int32_t ok = 1;
    uint8_t *swap = self->frameOld;
    self->frameOld = self->frameCur;
    self->frameCur = swap;

    int32_t pairs = static_cast<int32_t>(Uv8Rd16(stream));
    self->countA = pairs;
    int32_t rawBlocks = static_cast<int32_t>(Uv8Rd16(stream + 2));
    self->countB = rawBlocks;
    int32_t codedBlocks = static_cast<int32_t>(Uv8Rd16(stream + 4));
    self->countC = codedBlocks;
    int32_t bits = static_cast<int32_t>(Uv8Rd16(stream + 6));
    self->countD = bits;

    if (pairs != 0) {
        if (pairs > self->pairCap) {
            if (self->pairTable != nullptr) {
                free(self->pairTable);
                self->pairTable = nullptr;
                self->pairCap = 0;
            }
            if (self->offsetTable != nullptr) {
                free(self->offsetTable);
                self->offsetTable = nullptr;
            }
        }
        if (self->pairTable == nullptr) {
            self->pairTable = static_cast<int32_t *>(malloc(static_cast<uint32_t>(pairs) * 8));
            self->pairCap = pairs;
        }
        if (self->offsetTable == nullptr) {
            self->offsetTable = static_cast<int32_t *>(malloc(static_cast<uint32_t>(pairs) * 4));
            self->pairCap = pairs;
        }
        if (self->pairTable == nullptr) {
            ok = 0;
        }
        if (self->offsetTable == nullptr) {
            ok = 0;
        }
    }

    int32_t total = codedBlocks + rawBlocks;
    if (total != 0) {
        if (total > self->blockCap) {
            if (self->blockTable != nullptr) {
                free(self->blockTable);
                self->blockTable = nullptr;
            }
        }
        if (self->blockTable == nullptr) {
            self->blockTable = static_cast<uint8_t *>(malloc(static_cast<uint32_t>(total) << 4));
            self->blockCap = total;
        }
        if (self->blockTable == nullptr) {
            ok = 0;
        }
    }

    if (ok != 0) {
        // Bytes taken by the motion pairs: 2 fields of 10 bits per entry, rounded to dwords.
        int32_t pairBytes = (pairs * 20 + 0x1f) >> 3;
        pairBytes &= ~3;
        uint32_t fieldCount = static_cast<uint32_t>(pairs) * 2;
        const uint8_t *data = stream + 0xc;
        int32_t bitpos = static_cast<int32_t>(fieldCount * 10 - 10);
        int32_t *fill = self->pairTable + fieldCount;
        if (fieldCount != 0) {
            uint32_t n = fieldCount;
            do {
                uint32_t v = Uv8Rd32(data + (static_cast<uint32_t>(bitpos) >> 3));
                v >>= (bitpos & 7);
                bitpos -= 10;
                v &= 0x3ff;
                if (v & 0x200) {
                    v |= 0xfffffc00;
                }
                fill--;
                *fill = static_cast<int32_t>(v);
                n--;
            } while (n != 0);
        }
        const int32_t *rowBase = self->rowOffsets + self->fmt->height;
        for (int32_t i = 0; i < pairs; i++) {
            int32_t row = self->pairTable[i * 2 + 1];
            self->offsetTable[i] = rowBase[row] + self->pairTable[i * 2];
        }

        const uint8_t *pos = data + pairBytes;
        uint32_t rawBytes = static_cast<uint32_t>(rawBlocks) << 4;
        if (rawBytes != 0) {
            memcpy(self->blockTable, pos, rawBytes);
        }
        pos += rawBytes;

        uint32_t codedBytes = static_cast<uint32_t>(codedBlocks) * 8;
        if (codedBytes != 0) {
            uint8_t *out = self->blockTable + rawBytes;
            const uint8_t *in = pos;
            if (codedBlocks > 0) {
                int32_t k = codedBlocks;
                do {
                    uint32_t idx = Uv8Rd32(in + 4);
                    out[15] = in[idx & 3];
                    idx >>= 2;
                    out[14] = in[idx & 3];
                    idx >>= 2;
                    out[13] = in[idx & 3];
                    idx >>= 2;
                    out[12] = in[idx & 3];
                    idx >>= 2;
                    out[11] = in[idx & 3];
                    idx >>= 2;
                    out[10] = in[idx & 3];
                    idx >>= 2;
                    out[9] = in[idx & 3];
                    idx >>= 2;
                    out[8] = in[idx & 3];
                    idx >>= 2;
                    out[7] = in[idx & 3];
                    idx >>= 2;
                    out[6] = in[idx & 3];
                    idx >>= 2;
                    out[5] = in[idx & 3];
                    idx >>= 2;
                    out[4] = in[idx & 3];
                    idx >>= 2;
                    out[3] = in[idx & 3];
                    idx >>= 2;
                    out[2] = in[idx & 3];
                    idx >>= 2;
                    out[1] = in[idx & 3];
                    idx >>= 2;
                    out[0] = in[idx & 3];
                    in += 8;
                    out += 16;
                    k--;
                } while (k != 0);
            }
        }
        pos += codedBytes;

        uint32_t blockCount = (self->fmt->width * self->fmt->height) >> 4;
        uint32_t check = static_cast<uint32_t>(bits) * blockCount + 0x1f;
        if ((check & 0xffffffe0u) != 0) {
            UvUnpackBitfields(reinterpret_cast<uint32_t *>(self->indexArray), pos, static_cast<int32_t>(blockCount), bits);
        }
        UvCopyRows(self->indexArray, self->frameOld, self->frameCur, self->countA, self->offsetTable,
                   self->blockTable, self->fmt->width, static_cast<int32_t>(self->fmt->width >> 2),
                   static_cast<int32_t>(self->fmt->height >> 2));
    }
    return ok;
}
HOOK_FUNC(0x00489f80, UvDecodeFrameSetup);
