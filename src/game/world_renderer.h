// CWorldRenderer: the object at 0x0069e0c0 that redraws the world view (the same address the drag
// gump uses as the "world picker"). Fields from the assembly of 0x0042a490, 0x0042a4b0, 0x0042bed0 and
// the readers 0x0042c130 / 0x0042c3b0 (which read +0x00 / +0x04 / +0x08 through the absolute addresses).
// Also here: the ordered visible-cell table builder, the RLE sprite blitters and the roof helpers of the
// 0x0042a350 .. 0x0042ac60 and 0x0042bd90 .. 0x0042bed0 ranges. (Porting pass 2, batch 10.)
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

class CWorldRenderer {
public:
    // 0x0042a490: zero +0x00, +0x18, +0x1c and the 0x14 dwords at 0x004b7250. Returns this.
    CWorldRenderer *__thiscall Reset();
    // 0x0042a4b0: RET 4. Optionally blank `rc` ({left, top, right, bottom}, screen coordinates) in the back
    // buffer, then draw every object of the visible cells that intersects it (vtable slot 0x08).
    void __thiscall RedrawRect(int *rc);
    // 0x0042bed0: recompute the roof / floor hiding limits from the objects above the player.
    void __thiscall UpdateRoofHideLimit();

    int32_t clearFirst;     // +0x00  non-zero: RedrawRect blanks the rect first; also tested by 0x0042c130 / 0x0042c3b0
    int32_t hideZ;          // +0x04  z limit read by 0x0042c130 / 0x0042c3b0 (0x0069e0c4)
    int32_t roofLimit;      // +0x08  0x7f = nothing hidden (0x0069e0c8)
    int32_t roofLimit2;     // +0x0c
    uint8_t pad_10[8];
    int32_t unk_18;         // +0x18
    int32_t unk_1c;         // +0x1c
};

static_assert(offsetof(CWorldRenderer, hideZ) == 0x04, "CWorldRenderer::hideZ");
static_assert(offsetof(CWorldRenderer, roofLimit) == 0x08, "CWorldRenderer::roofLimit");
static_assert(offsetof(CWorldRenderer, roofLimit2) == 0x0c, "CWorldRenderer::roofLimit2");
static_assert(offsetof(CWorldRenderer, unk_18) == 0x18, "CWorldRenderer::unk_18");
static_assert(offsetof(CWorldRenderer, unk_1c) == 0x1c, "CWorldRenderer::unk_1c");

#define g_worldRenderer GLOBAL_AS(CWorldRenderer, g_worldPicker_storage)

// 0x0042a350: fill the visible-cell table (0x004b72a8, count at 0x004ba308) in diagonal scan order.
void __cdecl WorldBuildVisibleCellTable();
// 0x0042bea0: ECX = a, EDX = b ({left, top, right, bottom}); strict overlap, returns 0 / 1.
int __fastcall WorldRectsOverlap(const int *a, const int *b);
// 0x0042bd90: flood-fill over roof cells; ECX = running minimum z, EDX = x, stack (y, z), RET 8.
int __fastcall WorldRoofFloodFill(int minZ, int x, int y, int z);

// 0x0042a670: darken the pixels covered by a shadow RLE. ECX = mirror flag, EDX = x, stack (y, clip, rle), RET 0xc.
void __fastcall WorldDrawShadowRLE(int mirror, int x, int y, const int *clip, const uint8_t *rle);
// 0x0042a7f0: blit a paletted RLE sprite into the back buffer. ECX = mirror flag, EDX = hue mode,
// stack (x, y, clip, palette, rle), RET 0x14.
void __fastcall WorldBlitRLESprite(int mirror, unsigned mode, int x, int y, const int *clip, const uint16_t *pal,
                                   const uint8_t *rle);
// 0x0042ac60: does the RLE sprite cover pixel (ECX, EDX)? Stack (mirror, xorg, yorg, rle), RET 0x10.
int __fastcall WorldHitTestRLESprite(int px, int py, int mirror, int xorg, int yorg, const uint8_t *rle);
