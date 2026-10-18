// uV intro video player: the 8-bit indexed-frame codec (0x00489c0a - 0x0048aacb).
// Only the fields these procedures touch are declared. (Porting pass 3, batch v4.)
#pragma once

#include <cstddef>
#include <cstdint>

// The output-format record the player object points to (player +0x000). Pixel format of the destination.
struct UvOutFormat8 {
    uint32_t flags;    // +0x00  byte 0 is tested: bit 0x40 = 32-bit output (two copies of the pixel in a dword / dword table)
    uint8_t pad_04[0x1c];
    uint32_t redBits;   // +0x20  bits in the first palette byte's channel (shift = 8 - n)
    uint32_t greenBits; // +0x24
    uint32_t blueBits;  // +0x28
    uint32_t redShift;  // +0x2c  position of the first channel
    uint32_t greenShift; // +0x30
    uint32_t blueShift; // +0x34
    uint8_t pad_38[4];
    uint32_t bitsPerPixel; // +0x3c  8 = indexed output
    uint8_t pad_40[0x14];
    uint32_t height;    // +0x54
    uint32_t width;     // +0x58
};
static_assert(offsetof(UvOutFormat8, redBits) == 0x20, "UvOutFormat8");
static_assert(offsetof(UvOutFormat8, redShift) == 0x2c, "UvOutFormat8");
static_assert(offsetof(UvOutFormat8, blueShift) == 0x34, "UvOutFormat8");
static_assert(offsetof(UvOutFormat8, bitsPerPixel) == 0x3c, "UvOutFormat8");
static_assert(offsetof(UvOutFormat8, height) == 0x54, "UvOutFormat8");
static_assert(offsetof(UvOutFormat8, width) == 0x58, "UvOutFormat8");

// The video player object (ECX of the frame entries). Large; only these fields are declared.
struct UvPlayer8 {
    UvOutFormat8 *fmt;      // +0x000
    uint8_t pad_004[0x3f0];
    int32_t streamExtra;    // +0x3f4  used by the alloc entry to find the RefPack stream
    int32_t countA;         // +0x3f8  stream u16 #0: number of motion (offset) entries
    int32_t countB;         // +0x3fc  stream u16 #1: block-table entries stored as 16 raw bytes
    int32_t countC;         // +0x400  stream u16 #2: block-table entries stored as 4 colours + 16 two-bit indices
    int32_t countD;         // +0x404  stream u16 #3: bits per index in the bitfield area
    int32_t pairCap;        // +0x408  capacity of pairTable / offsetTable in entries
    int32_t blockCap;       // +0x40c  capacity of blockTable in 16-byte entries
    uint8_t *frameCur;      // +0x410  frame buffer (swapped with frameOld each frame)
    uint8_t *frameOld;      // +0x414
    int32_t *indexArray;    // +0x418  unpacked per-4x4-block indices (dwords)
    int32_t *rowOffsets;    // +0x41c  2*height entries: -(w*h) + k*w
    uint8_t *blockTable;    // +0x420
    int32_t *pairTable;     // +0x424
    int32_t *offsetTable;   // +0x428
    uint8_t palette[0x300]; // +0x42c  3 bytes per entry
    uint32_t colorTable[256]; // +0x72c
};
static_assert(offsetof(UvPlayer8, streamExtra) == 0x3f4, "UvPlayer8");
static_assert(offsetof(UvPlayer8, countA) == 0x3f8, "UvPlayer8");
static_assert(offsetof(UvPlayer8, countD) == 0x404, "UvPlayer8");
static_assert(offsetof(UvPlayer8, pairCap) == 0x408, "UvPlayer8");
static_assert(offsetof(UvPlayer8, blockCap) == 0x40c, "UvPlayer8");
static_assert(offsetof(UvPlayer8, frameCur) == 0x410, "UvPlayer8");
static_assert(offsetof(UvPlayer8, indexArray) == 0x418, "UvPlayer8");
static_assert(offsetof(UvPlayer8, rowOffsets) == 0x41c, "UvPlayer8");
static_assert(offsetof(UvPlayer8, blockTable) == 0x420, "UvPlayer8");
static_assert(offsetof(UvPlayer8, pairTable) == 0x424, "UvPlayer8");
static_assert(offsetof(UvPlayer8, offsetTable) == 0x428, "UvPlayer8");
static_assert(offsetof(UvPlayer8, palette) == 0x42c, "UvPlayer8");
static_assert(offsetof(UvPlayer8, colorTable) == 0x72c, "UvPlayer8");
