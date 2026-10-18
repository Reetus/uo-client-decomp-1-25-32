// CFontManager: the font/text drawing object (one static instance at 0x0064cde8).
// Layout partial: the first 0x1a dwords are the per-font glyph blocks. See
// decomp/pass2/0x004328e0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

class CFontManager {
public:
    // 0x004328e0: word-wrap text to maxWidth, measuring each line with 0x004327b0.
    // Returns 1 with the widest line width, total height and the first line base filled in;
    // 0 when the font is invalid or text is null.
    int __thiscall MeasureTextWrapped(int font, char *text, int maxWidth, int *outW, int *outH, int *outBase, int indent);

    // 0x00431e00: constructor (static initializer): clear the block table and the 0x10000-entry
    // table at +0x64, load fonts.mul, set the hue flag of fonts 0..8 from the table at 0x004a8188.
    CFontManager *__thiscall Ctor();
    // 0x00431e60: destructor: free every glyph bitmap, block and the table at +0x64.
    void __thiscall Dtor();
    // 0x00431f10: read fonts.mul (25 fonts of 0xe0 glyphs: width, height, baseline, 16-bit pixels).
    // Returns 0 with a message box when the file is missing.
    int __thiscall LoadFonts();
    // 0x004320a0: font index valid (0..0x19) and loaded.
    int __thiscall HasFont(int font);
    // 0x004320d0: font loaded and character ch (0x20..0xff) has a glyph with nonzero width.
    int __thiscall HasGlyph(int font, int ch);
    // 0x00432130: draw one line of text (RET 0x20); the profanity filter is applied to a copy when
    // enabled. Returns 1, or 0 with a message box when the font is not loaded. (Named DrawTextPlain
    // because DrawText is a windows.h macro.)
    int __thiscall DrawTextPlain(int x, int y, int font, int hue, char *text, uint16_t *dest, int stride, const int *clip);
    // 0x00432260: word-wrap and draw (RET 0x2c). Returns the total height drawn
    // (lineHeight per line when nonzero, else the measured line height).
    int __thiscall DrawTextWrapped(int x, int y, int font, int hue, char *text, int maxWidth, uint16_t *dest, int stride,
                                   const int *clip, int indent, int lineHeight);
    // 0x00432550: draw one line, shortened with "..." so that it ends within clip[2] (RET 0x20).
    int __thiscall DrawTextClipped(int x, int y, int font, int hue, char *text, uint16_t *dest, int stride, const int *clip);
    // 0x004327b0: width, height and base of one line (RET 0x14). Returns 0 when the font is not loaded.
    int __thiscall MeasureText(int font, char *text, int *outW, int *outH, int *outBase);
    // 0x00432ba0: index of the last character that fits in maxWidth (the last space, or the
    // character before the overflowing one), or -1 when the whole text fits (RET 0xc).
    int __thiscall FindWrapBreak(int font, const char *text, int maxWidth);

    // 0x004335d0: advance width of one character (0 when the font or character is invalid).
    int __thiscall GetGlyphWidth(int font, int ch);
    // 0x00433620: summed advance of the first n characters of text.
    int __thiscall GetTextWidthN(int font, int n, const char *text);
    // 0x004336c0: pixel position of character charIndex in text wrapped to maxWidth.
    void __thiscall GetCaretPos(int font, int charIndex, char *text, int maxWidth, int *outX, int *outY);
    // 0x00433a50: number of characters of text that fit in width pixels.
    int __thiscall CharsFitting(int font, const char *text, int width);
    // 0x00433b30: character index nearest to (px, py) in text wrapped to maxWidth.
    int __thiscall CharIndexAtPoint(int font, const char *text, int maxWidth, int px, int py);

    int32_t fontBlock[0x1a];    // +0x00  per-font glyph block pointers (0 = font not loaded)
};

static_assert(offsetof(CFontManager, fontBlock) == 0x00, "CFontManager::fontBlock");

#define g_fontMgr GLOBAL_AS(CFontManager, g_fontMgr_storage)
