// CFontManager width / caret helpers and the CFontBlock glyph block (font drawing area).
// Glyph pointer for character ch is at block + ch * 4 - 0x7c (CFontBlock::glyph[ch - 0x20]).
#include <cstring>

#include "../runtime/ported.h"
#include "font_block.h"
#include "font_manager.h"
#include "crt.h"
#include "art_row_blits_ports.h"
#include "hues_ports.h"
#include "globals.h"

// 0x004327b0: measure one line (font, text, &w, &h, &base). ECX = font manager.
#define FM_MeasureText(self, a1, a2, a3, a4, a5) (reinterpret_cast<CFontManager *>(self)->MeasureText(a1, a2, a3, a4, a5))

// 0x004764c0 plain row copy, 0x004765a0 hued row copy (hue index in 0x0074796c),
// 0x0047e700 hue-table row copy: __fastcall(dst, src, count[, hue]), callee pops.
typedef void(__fastcall *FmRowBlit)(uint16_t *dst, const uint16_t *src, int count);
typedef void(__fastcall *FmRowBlitHue)(uint16_t *dst, const uint16_t *src, int count, int hue);
#define FM_ROW_PLAIN reinterpret_cast<FmRowBlit>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitCopyNonZero)))
#define FM_ROW_HUED reinterpret_cast<FmRowBlit>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitLookupNonZero)))
#define FM_ROW_HUETABLE reinterpret_cast<FmRowBlitHue>(static_cast<uintptr_t>(PORT_ADDR(&HueCopyPixels)))

static inline bool fm_font_ok(const CFontManager *fm, int font)
{
    return font <= 0x19 && font >= 0 && fm->fontBlock[font] != 0;
}

// Advance width of character c in a loaded font block (0 outside 0x20..0xff).
static inline int fm_glyph_width(const CFontManager *fm, int font, int c)
{
    if (fm_font_ok(fm, font) && c >= 0x20 && c - 0x20 < 0xe0) {
        const CFontBlock *block = reinterpret_cast<const CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(fm->fontBlock[font])));
        const uint16_t *glyph = reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(block->glyph[c - 0x20])));
        return *glyph;
    }
    return 0;
}

// 0x004335d0  size 0x4d
// Advance width of one character; 0 for an unloaded font or a character outside 0x20..0xff.
int __thiscall CFontManager::GetGlyphWidth(int font, int ch)
{
    int ok;
    if (font <= 0x19 && font >= 0)
        ok = fontBlock[font] != 0;
    else
        ok = 0;
    if (ok && ch >= 0x20 && ch - 0x20 < 0xe0) {
        const CFontBlock *block = reinterpret_cast<const CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(fontBlock[font])));
        const uint16_t *glyph = reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(block->glyph[ch - 0x20])));
        return *glyph;
    }
    return 0;
}
HOOK_METHOD(0x004335d0, CFontManager, GetGlyphWidth);

// 0x00433620  size 0x96
// Sum of the advances of the first n characters of text (signed chars).
int __thiscall CFontManager::GetTextWidthN(int font, int n, const char *text)
{
    int sum = 0;
    int i = 0;

    if (!fm_font_ok(this, font))
        return 0;
    if (n > 0) {
        do {
            sum += fm_glyph_width(this, font, text[i]);
            i++;
        } while (i < n);
    }
    return sum;
}
HOOK_METHOD(0x00433620, CFontManager, GetTextWidthN);

// 0x004336c0  size 0x38a
// Wraps text to maxWidth like 0x004328e0 (no space skipping) and gives the pixel position
// (x within its line, y = height of the lines above) of character charIndex.
// The SEH frame of the original (try level around operator new) is omitted.
void __thiscall CFontManager::GetCaretPos(int font, int charIndex, char *text, int maxWidth, int *outX, int *outY)
{
    char line[200];
    int maxW = 0;
    int yAcc = 0;
    int firstBase = 0;
    int consumed = 0;
    int lastSpace, brk, len, width, i, cnt, w, h, base, rem, k, n, lw;
    char *buf;
    char c;

    *outX = 0;
    *outY = 0;
    if (!fm_font_ok(this, font))
        return;
    if (text == nullptr)
        return;

    buf = static_cast<char *>(::operator new(strlen(text) + 1));   // 0x0048ad70
    strcpy(buf, text);

    do {
        lastSpace = -1;
        width = 0;
        len = static_cast<int>(strlen(buf));
        i = 0;
        brk = -1;
        if (len > 0) {
            for (;;) {
                c = buf[i];
                width += fm_glyph_width(this, font, c);
                if (c == ' ' && i > 1)
                    lastSpace = i - 1;
                if (width > maxWidth) {
                    if (lastSpace != -1)
                        brk = lastSpace;
                    else
                        brk = i - 1;
                    break;
                }
                i++;
                if (i >= len)
                    break;
            }
        }

        if (brk != -1) {
            cnt = brk + 1;
            memcpy(line, buf, static_cast<size_t>(cnt));
            line[cnt] = 0;

            if (charIndex >= consumed && charIndex < consumed + cnt) {
                if (!fm_font_ok(this, font)) {
                    *outX = 0;
                    *outY = yAcc;
                } else {
                    lw = 0;
                    n = charIndex - consumed;
                    for (k = 0; k < n; k++)
                        lw += fm_glyph_width(this, font, line[k]);
                    *outX = lw;
                    *outY = yAcc;
                }
            }

            FM_MeasureText(this, font, line, &w, &h, &base);
            if (w > maxW)
                maxW = w;
            yAcc += h;
            if (firstBase == 0)
                firstBase = base;

            rem = static_cast<int>(strlen(buf)) - cnt;
            k = 0;
            if (rem > 0) {
                do {
                    buf[k] = buf[cnt + k];
                    k++;
                } while (k < rem);
            }
            buf[k] = 0;
            consumed += cnt;
        }
    } while (brk != -1);

    if (charIndex >= consumed) {
        if (!fm_font_ok(this, font)) {
            *outX = 0;
            *outY = yAcc;
        } else {
            lw = 0;
            n = charIndex - consumed;
            for (k = 0; k < n; k++)
                lw += fm_glyph_width(this, font, buf[k]);
            *outX = lw;
            *outY = yAcc;
        }
    }

    FM_MeasureText(this, font, buf, &w, &h, &base);   // result unused
    operator delete(buf);                                    // operator delete 0x0048ad60
}
HOOK_METHOD(0x004336c0, CFontManager, GetCaretPos);

// 0x00433a50  size 0xdc
// Number of characters of text that fit in width pixels.
int __thiscall CFontManager::CharsFitting(int font, const char *text, int width)
{
    int len = static_cast<int>(strlen(text));
    int i = 0;
    int rem = width;
    int gw = fm_glyph_width(this, font, text[0]);

    if (width > gw) {
        for (;;) {
            if (i >= len)
                break;
            rem -= gw;
            i++;
            if (i < len)
                gw = fm_glyph_width(this, font, text[i]);
            if (!(rem > gw))
                break;
        }
    }
    return i;
}
HOOK_METHOD(0x00433a50, CFontManager, CharsFitting);

// 0x00433b30  size 0x57
// Tries prefix lengths len, len-1, ... with GetCaretPos until the caret position is at or
// before (px, py); returns that prefix length (or the last one tried).
int __thiscall CFontManager::CharIndexAtPoint(int font, const char *text, int maxWidth, int px, int py)
{
    int y, x;
    int idx = static_cast<int>(strlen(text));

    do {
        GetCaretPos(font, idx, const_cast<char *>(text), maxWidth, &x, &y);
        idx--;
    } while (idx >= 0 && (x > px || y > py));
    return idx + 1;
}
HOOK_METHOD(0x00433b30, CFontManager, CharIndexAtPoint);

// 0x00433b90  size 0x22
CFontBlock::CFontBlock()
{
    memset(glyph, 0, sizeof(glyph));
    for (int i = 0; i < 0xe0; i++)
        baseline[i] = 0;
    hued = 0;
}
HOOK_CTOR(0x00433b90, CFontBlock, ());

// 0x00433bc0  size 0x28
CFontBlock::~CFontBlock()
{
    for (int i = 0; i < 0xe0; i++) {
        if (glyph[i] != 0) {
            operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(glyph[i]))));   // operator delete 0x0048ad60
            glyph[i] = 0;
        }
    }
}
HOOK_DTOR(0x00433bc0, CFontBlock);

// Non-transparent pixels of a 555 glyph row are converted to 565 in place.
static inline void fm_fix565(uint16_t *dst, const uint16_t *src, int w)
{
    if (g_is565 != 0 && w > 0) {
        for (int n = 0; n < w; n++) {
            if (src[n] != 0)
                dst[n] = static_cast<uint16_t>(((dst[n] & 0xffe0) << 1) | (dst[n] & 0x1f));
        }
    }
}

// 0x00433bf0  size 0x27d
// Draw glyph ch at (x, y) (y is the baseline row of the line) into the 16-bit buffer dest,
// clipped to clip = {left, top, right, bottom}. Rows are copied with the plain blitter (hue 0),
// the hue-table blitter (hued font) or the hued blitter (hue index in 0x0074796c).
// Returns the glyph width; 0 when the character has no glyph or lies outside the clip columns.
int __thiscall CFontBlock::DrawGlyph(int x, int y, int hue, int ch, uint16_t *dest, int stride, const int *clip)
{
    int idx, w, h, skip, lastRow, rows, bottom;
    int32_t bitmap;
    const uint16_t *src;
    uint16_t *dst;

    if (ch < 0x20)
        return 0;
    idx = ch - 0x20;
    if (idx < 0xe0) {
        const uint16_t *g = reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(glyph[idx])));
        w = g[0];
        h = g[1];
    } else {
        w = 0;
        h = 0;
    }
    if (w == 0)
        return 0;

    y = baseline[idx] - h + y + 1;
    bitmap = glyph[idx];
    if (bitmap == 0)
        return 0;
    dst = dest + (y * stride + x);
    src = reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(bitmap))) + 2;   // skip u16 width, height

    if (x < clip[0])
        return 0;
    if (x + w - 1 >= clip[2])
        return 0;

    if (y < clip[1]) {
        skip = clip[1] - y;
        src += w * skip;
        dst += skip * stride;
    } else {
        skip = 0;
    }
    bottom = clip[3];
    if (y + h >= bottom)
        lastRow = bottom - y;
    else
        lastRow = h;
    lastRow--;

    if (hue == 0) {
        if (skip > lastRow)
            return w;
        rows = lastRow - skip + 1;
        do {
            FM_ROW_PLAIN(dst, src, w);
            fm_fix565(dst, src, w);
            src += w;
            dst += stride;
            rows--;
        } while (rows != 0);
        return w;
    }

    if (hued != 0) {
        if (skip > lastRow)
            return w;
        rows = lastRow - skip + 1;
        do {
            FM_ROW_HUETABLE(dst, src, w, hue - 1);
            fm_fix565(dst, src, w);
            src += w;
            dst += stride;
            rows--;
        } while (rows != 0);
        return w;
    }

    g_hueOverride = hue - 1;
    if (skip > lastRow)
        return w;
    rows = lastRow - skip + 1;
    do {
        FM_ROW_HUED(dst, src, w);
        fm_fix565(dst, src, w);
        src += w;
        dst += stride;
        rows--;
    } while (rows != 0);
    return w;
}
HOOK_METHOD(0x00433bf0, CFontBlock, DrawGlyph);
