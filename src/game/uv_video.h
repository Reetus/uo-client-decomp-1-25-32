// uV intro video player: the DirectDraw surface layer (0x00485210 - 0x00486e76).
// Only the fields these procedures touch are declared. (Porting pass 3, batch v2.)
#pragma once

#include <cstddef>
#include <cstdint>

// The output-surface record the player object points to (player +0x000).
struct UvSurf {
    uint32_t flags;        // +0x00  0x800 software buffers, 0x4000000 external palette, 0x40 doubled width,
                           //        0x30 line doubling, 0x20 / 0x10 scan-line modes, 0x80 / 0x100 explicit size,
                           //        0x4 / 0x8 centre x / y, 0x1 / 0x2 add offsets, 0x100000 keep brightness
    uint8_t pad_04[4];
    void *dd;              // +0x08  IDirectDraw
    void *primary;         // +0x0c  IDirectDrawSurface
    void *palette;         // +0x10  IDirectDrawPalette
    uint8_t *palEntries;   // +0x14  256 PALETTEENTRY
    void *surf0;           // +0x18  offscreen surface
    void *surf1;           // +0x1c  offscreen surface
    int32_t rBits;         // +0x20
    int32_t gBits;         // +0x24
    int32_t bBits;         // +0x28
    int32_t rShift;        // +0x2c
    int32_t gShift;        // +0x30
    int32_t bShift;        // +0x34
    uint32_t maskAll;      // +0x38
    int32_t bpp;           // +0x3c
    uint8_t *buf0;         // +0x40  software frame buffer
    uint8_t *buf1;         // +0x44
    uint8_t pad_48[0xc];
    uint32_t height;       // +0x54
    uint32_t width;        // +0x58
    int32_t offX;          // +0x5c
    int32_t offY;          // +0x60
    uint32_t sizeW;        // +0x64
    uint32_t sizeH;        // +0x68
    int32_t brightness;    // +0x6c
    uint8_t pad_70[4];
    int32_t disabled;      // +0x74
};
static_assert(offsetof(UvSurf, dd) == 0x08, "UvSurf");
static_assert(offsetof(UvSurf, surf0) == 0x18, "UvSurf");
static_assert(offsetof(UvSurf, rBits) == 0x20, "UvSurf");
static_assert(offsetof(UvSurf, bShift) == 0x34, "UvSurf");
static_assert(offsetof(UvSurf, bpp) == 0x3c, "UvSurf");
static_assert(offsetof(UvSurf, buf0) == 0x40, "UvSurf");
static_assert(offsetof(UvSurf, height) == 0x54, "UvSurf");
static_assert(offsetof(UvSurf, width) == 0x58, "UvSurf");
static_assert(offsetof(UvSurf, brightness) == 0x6c, "UvSurf");
static_assert(offsetof(UvSurf, disabled) == 0x74, "UvSurf");

// The video player object (ECX of the surface entries).
struct UvVideo {
    UvSurf *surf;          // +0x000
    int32_t winW;          // +0x004  primary surface width
    int32_t winH;          // +0x008  primary surface height
    uint8_t pad_00c[0x10];
    int32_t inited;        // +0x01c
    int32_t flag20;        // +0x020
    int32_t flag24;        // +0x024
    int32_t quality;       // +0x028
    uint8_t pad_02c[0x39c];
    uint32_t state[3];     // +0x3c8  codec state words (cleared per frame)
    uint8_t pad_3d4[0x1c];
    int32_t hdr;           // +0x3f0
    int32_t palCount;      // +0x3f4
    uint8_t pad_3f8[0x18];
    void *mem410;          // +0x410
    void *mem414;          // +0x414
    void *mem418;          // +0x418
    void *mem41c;          // +0x41c
    void *mem420;          // +0x420
    void *mem424;          // +0x424
    void *mem428;          // +0x428
    uint8_t palette[0x300]; // +0x42c
};
static_assert(offsetof(UvVideo, winW) == 0x04, "UvVideo");
static_assert(offsetof(UvVideo, inited) == 0x1c, "UvVideo");
static_assert(offsetof(UvVideo, quality) == 0x28, "UvVideo");
static_assert(offsetof(UvVideo, state) == 0x3c8, "UvVideo");
static_assert(offsetof(UvVideo, hdr) == 0x3f0, "UvVideo");
static_assert(offsetof(UvVideo, palCount) == 0x3f4, "UvVideo");
static_assert(offsetof(UvVideo, mem410) == 0x410, "UvVideo");
static_assert(offsetof(UvVideo, mem428) == 0x428, "UvVideo");
static_assert(offsetof(UvVideo, palette) == 0x42c, "UvVideo");

// A chunk handed to the frame decoder (EDX of 0x00486850).
struct UvChunk {
    const uint8_t *data;   // +0x00
    uint8_t pad_04[8];
    int32_t type;          // +0x0c  1 header + palette, 2 8-bit frame, 3 / 4 DCT frames
};
static_assert(offsetof(UvChunk, type) == 0x0c, "UvChunk");

// Ported procedures of this batch that call each other (uv_video_blocks.cpp, uv_video_tables.cpp).
void __fastcall UV_ReadHeaderPalette(UvVideo *o, const uint8_t *data);
void __fastcall UV_CopyMotionBlock16(uint16_t *dst, const uint16_t *src, int32_t x, int32_t y, int32_t width, int32_t dx, int32_t dy);
void __fastcall UV_CopyMotionBlock32(uint32_t *dst, const uint32_t *src, int32_t x, int32_t y, int32_t width, int32_t dx, int32_t dy);
void __fastcall UV_WalkMacroblocks16(UvVideo *o, const uint8_t *src, uint8_t *dst, uint8_t *unused, uint32_t pitch);
void __fastcall UV_WalkMacroblocks32(UvVideo *o, const uint8_t *src, uint8_t *dst, uint8_t *unused, uint32_t pitch);
void __cdecl UV_SetBitStream(const uint8_t *stream);
