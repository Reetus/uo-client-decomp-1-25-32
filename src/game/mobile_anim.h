// A 64x128 16-bit sprite buffer with eight (v * 0x40 + u) index maps, used to
// draw an animation frame with a per-direction offset onto the back buffer.
// ? The original class name is unknown; the name here is a placeholder.
// Layout from decomp/pass2/0x00470520.cpp, 0x00470870.cpp and 0x00470b50.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

// One entry of the table at 0x004ae760 (stride 0x14, terminated by id == 0).
struct AnimOverlayEntry {
    int32_t id;         // +0x00  object model id
    int32_t enabled;    // +0x04  entry only matches when non-zero
    int32_t offsetA;    // +0x08  y offset for directions other than 0 / 6
    int32_t offsetB;    // +0x0c  y offset for directions 0 and 6
    int32_t offsetC;    // +0x10  x extent used to clip the matching object (0 = none)
};
static_assert(sizeof(AnimOverlayEntry) == 0x14, "AnimOverlayEntry size");

class CAnimOverlayBuffer {
public:
    // 0x00470520  fill the eight index maps; returns this
    int __thiscall BuildIndexMaps();
    // 0x00470870  draw the buffer onto the back buffer, then redraw the matching map object clipped. obj: CObj with x/y/z at +0x20/+0x22/+0x24
    void __thiscall DrawOverlay(void *obj);
    // 0x00470b50  decode one RLE frame into pixels (mode: 0 plain, 1..0x7fff shaded, 0x8000|n grey-only remap)
    void __thiscall DecodeFrame(unsigned int mode, uint16_t *palette, uint32_t *rle);

    // Second pass, mobile_cached_draw.cpp. The one instance of this buffer is at 0x007226a0.
    // 0x00470aa0  RET 8: 16-bit pixel under screen point (px, py) after the last draw, 0 outside the 64x128 box.
    // Moves `y` by the table's offset each call.
    int __thiscall PixelAt(int px, int py);
    // 0x00470fa0  RET 4: *facing &= 7, then the nearest facing that is set in the table entry's mask at +4
    // (tried in the order +0, +1, -1, +2, -2, +3, -3, -4 from the starting facing).
    void __thiscall NearestAllowedFacing(int *facing);

    int32_t tableIndex;             // +0x00  index into the AnimOverlayEntry table
    int32_t *clip;                  // +0x04  {left, top, right, bottom}
    uint16_t pixels[64 * 128];      // +0x08
    int32_t maps[0x20000 / 4];      // +0x4008  eight (v * 0x40 + u) maps, see BuildIndexMaps
    int32_t x;                      // +0x24008
    int32_t y;                      // +0x2400c
    int32_t unk_24010;              // +0x24010  mirror flag (DecodeFrame)
    int32_t direction;              // +0x24014
};

static_assert(offsetof(CAnimOverlayBuffer, clip) == 0x04, "CAnimOverlayBuffer::clip");
static_assert(offsetof(CAnimOverlayBuffer, pixels) == 0x08, "CAnimOverlayBuffer::pixels");
static_assert(offsetof(CAnimOverlayBuffer, maps) == 0x4008, "CAnimOverlayBuffer::maps");
static_assert(offsetof(CAnimOverlayBuffer, x) == 0x24008, "CAnimOverlayBuffer::x");
static_assert(offsetof(CAnimOverlayBuffer, y) == 0x2400c, "CAnimOverlayBuffer::y");
static_assert(offsetof(CAnimOverlayBuffer, unk_24010) == 0x24010, "CAnimOverlayBuffer::unk_24010");
static_assert(offsetof(CAnimOverlayBuffer, direction) == 0x24014, "CAnimOverlayBuffer::direction");
