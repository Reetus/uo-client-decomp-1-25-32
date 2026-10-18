// CFontBlock: the glyph block of one font (what CFontManager::fontBlock[i] points to).
// Zeroed by the constructor 0x00433b90; glyph bitmaps are freed by the destructor 0x00433bc0.
#pragma once

#include <cstddef>
#include <cstdint>

struct CFontBlock {
    // 0x00433b90: zero both tables and the flag. new CFontBlock() (operator new(0x704)) in LoadVerdata; LoadFonts
    // has the constructor expanded inline in the original and keeps it expanded.
    CFontBlock();
    // 0x00433bc0: free every glyph bitmap and clear its slot.
    ~CFontBlock();
    // 0x00433bf0: draw the glyph of ch at (x, y) clipped to clip (left, top, right, bottom),
    // hue 0 = plain; returns the glyph width, 0 when nothing was drawn.
    int __thiscall DrawGlyph(int x, int y, int hue, int ch, uint16_t *dest, int stride, const int *clip);

    int32_t hued;               // +0x000  nonzero: hue is applied by the hue-copy blitter (0x0047e700)
    int32_t glyph[0xe0];        // +0x004  glyph bitmaps for characters 0x20..0xff: u16 width, u16 height, pixels
    int32_t baseline[0xe0];     // +0x384  per-glyph baseline row
};

static_assert(offsetof(CFontBlock, glyph) == 0x04, "CFontBlock::glyph");
static_assert(offsetof(CFontBlock, baseline) == 0x384, "CFontBlock::baseline");
static_assert(sizeof(CFontBlock) == 0x704, "CFontBlock size");
