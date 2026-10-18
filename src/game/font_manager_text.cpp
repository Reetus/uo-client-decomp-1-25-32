// CFontManager: construction, font loading and the text measuring / drawing entry points
// (0x00431e00 .. 0x00432c70). The glyph block of font f is fontBlock[f] (CFontBlock): the glyph
// pointer of character ch is at block + ch * 4 - 0x7c, a glyph starts with u16 width, u16 height;
// the baseline of ch is the dword at block + ch * 4 + 0x304.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "font_block.h"
#include "font_manager.h"
#include "globals.h"
#include "crt.h"
#include "client_misc_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "object_manager_ops_ports.h"


// 0x00478cd0: option test (stub returning 0); the destructor calls it twice.
#define IsOptionSet() (Misc_Return0())
// 0x00482c70: message box (ECX = text, EDX = caption, stack = exit flag), RET 4.
#define Fatal_Box(a1, a2, a3) (ShowMessageBoxAndExit(a1, a2, a3))
// 0x00457460: profanity filter, in place (ECX = string).
#define ProfanityApply(o) PORT_THISCALL0(&Profanity_Apply, void, (o))()
// 0x00433bf0 CFontBlock::DrawGlyph (ported in font_manager_ops.cpp).

static inline int fmt_font_ok(const CFontManager *fm, int font)
{
    return (font <= 0x19 && font >= 0) ? (fm->fontBlock[font] != 0) : 0;
}

static inline const uint16_t *fmt_glyph(const CFontManager *fm, int font, int ch)
{
    const uint8_t *block = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fm->fontBlock[font])));
    return *reinterpret_cast<const uint16_t *const *>(block + ch * 4 - 0x7c);
}

// Advance width of ch as the inlined code computes it (0 for an unloaded font or ch outside 0x20..0xff).
static inline int fmt_glyph_width(const CFontManager *fm, int font, int ch)
{
    if (fmt_font_ok(fm, font) != 0 && ch >= 0x20 && ch - 0x20 < 0xe0)
        return fmt_glyph(fm, font, ch)[0];
    return 0;
}

static inline int fmt_block_ok(int font, const CFontBlock *block)
{
    return (font <= 0x19 && font >= 0) ? (block != 0) : 0;
}

static inline CFontBlock *fmt_block(const CFontManager *fm, int font)
{
    return reinterpret_cast<CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(fm->fontBlock[font])));
}

// 0x00431e00  size 0x5c
CFontManager *__thiscall CFontManager::Ctor()
{
    memset(this, 0, 0x19 * sizeof(int32_t));
    int32_t *table = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(this) + 0x64);
    for (int j = 0; j < 0x10000; j++)
        table[j] = 0;
    LoadFonts();   // 0x00431f10
    for (int i = 0; i < 9; i++) {
        int valid;
        if (i <= 0x19 && i >= 0)
            valid = (fontBlock[i] != 0);
        else
            valid = 0;
        if (valid != 0)
            *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fontBlock[i]))) = g_fontHuedFlags[i];
    }
    return this;
}
HOOK_METHOD(0x00431e00, CFontManager, Ctor);

// 0x00431e60  size 0xaa
// ? The two calls to 0x00478cd0 at the top are kept as they are (the second only when the flag at
// 0x004bab24 is set); that routine is a stub returning 0.
void __thiscall CFontManager::Dtor()
{
    if (PORT_THISCALL0(&Misc_Return0, int, this)() != 0) {   // 0x00478cd0
        if (g_dtorFlag != 0)
            PORT_THISCALL0(&Misc_Return0, int, this)();   // 0x00478cd0
    }
    int32_t *slot = fontBlock;
    for (int n = 0x19; n != 0; n--) {
        int32_t blockAddr = *slot;
        if (blockAddr != 0) {
            int32_t *glyphs = reinterpret_cast<int32_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(blockAddr)) + 4);
            for (int g = 0; g < 0xe0; g++) {
                if (glyphs[g] != 0) {
                    operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(glyphs[g]))));
                    glyphs[g] = 0;
                }
            }
            operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(blockAddr))));
            *slot = 0;
        }
        slot++;
    }
    int32_t *table = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(this) + 0x64);
    for (int i = 0; i < 0x10000; i++) {
        if (table[i] != 0) {
            operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(table[i]))));
            table[i] = 0;
        }
    }
}
HOOK_METHOD(0x00431e60, CFontManager, Dtor);

// 0x00431f10  size 0x18d
// fonts.mul: for each of 25 fonts one byte (0 = font absent), then 0xe0 glyphs of
// {width, height, baseline} bytes followed by width * height 16-bit pixels.
int __thiscall CFontManager::LoadFonts()
{
    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("fonts.mul")), g_fopenReadMode);
    if (fp == nullptr) {
        Fatal_Box("Missing Font!", "Error", 0);   // 0x00482c70
        return 0;
    }
    int32_t *slot = fontBlock;
    for (int n = 0x19; n != 0; n--) {
        int tmp;
        fread(&tmp, 1, 1, fp);
        if ((tmp & 0xff) != 0) {
            CFontBlock *block = static_cast<CFontBlock *>(::operator new(0x704));   // operator new, 0x0048ad70
            CFontBlock *made;
            if (block != nullptr) {
                memset(block->glyph, 0, sizeof(block->glyph));
                for (int i = 0; i < 0xe0; i++)
                    block->baseline[i] = 0;
                block->hued = 0;
                made = block;
            } else {
                made = nullptr;
            }
            *slot = static_cast<int32_t>(reinterpret_cast<uintptr_t>(made));
            for (int off = 4; off < 0x384; off += 4) {
                int height;
                int width;
                fread(&width, 1, 1, fp);
                fread(&height, 1, 1, fp);
                fread(&tmp, 1, 1, fp);
                CFontBlock *cur = reinterpret_cast<CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(*slot)));
                *reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(cur) + off + 0x380) = tmp & 0xff;
                uint8_t *glyph = static_cast<uint8_t *>(::operator new((height & 0xff) * (width & 0xff) * 2 + 4));   // operator new, 0x0048ad70
                cur = reinterpret_cast<CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(*slot)));
                *reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(cur) + off) = static_cast<int32_t>(reinterpret_cast<uintptr_t>(glyph));
                *reinterpret_cast<uint16_t *>(glyph) = static_cast<uint8_t>(width);
                *reinterpret_cast<uint16_t *>(glyph + 2) = static_cast<uint8_t>(height);
                fread(glyph + 4, static_cast<size_t>((height & 0xff) * (width & 0xff)), 2, fp);
            }
        }
        slot++;
    }
    fclose(fp);
    return 1;
}
HOOK_METHOD(0x00431f10, CFontManager, LoadFonts);

// 0x004320a0  size 0x28
int __thiscall CFontManager::HasFont(int font)
{
    if (font <= 0x19 && font >= 0)
        return fontBlock[font] != 0;
    return 0;
}
HOOK_METHOD(0x004320a0, CFontManager, HasFont);

// 0x004320d0  size 0x5f
int __thiscall CFontManager::HasGlyph(int font, int ch)
{
    int width;
    if (fmt_font_ok(this, font) != 0 && ch >= 0x20 && ch - 0x20 < 0xe0)
        width = fmt_glyph(this, font, ch)[0];
    else
        width = 0;
    return width != 0;
}
HOOK_METHOD(0x004320d0, CFontManager, HasGlyph);


// 0x00432130  size 0x125
// Draw each character with CFontBlock::DrawGlyph, advancing by its width; stops at the first
// character that draws nothing. The text is copied and run through the profanity filter first
// when the option at 0x0069e624 is on and no input-control redraw is in progress.
// The font index is not range checked here (fontBlock[font] is read directly).
int __thiscall CFontManager::DrawTextPlain(int x, int y, int font, int hue, char *text, uint16_t *dest, int stride, const int *clip)
{
    int copied = 0;
    char *buf;

    if (g_profanityOn != 0 && g_inputRedrawing == 0) {
        buf = static_cast<char *>(::operator new(strlen(text) + 1));   // operator new, 0x0048ad70
        strcpy(buf, text);
        copied = 1;
        text = buf;
        ProfanityApply(buf);   // 0x00457460
    } else {
        buf = text;
    }

    CFontBlock *block = reinterpret_cast<CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(fontBlock[font])));
    if (block == nullptr) {
        Fatal_Box("Missing Font!", "Error", 0);   // 0x00482c70
        if (copied != 0)
            operator delete(buf);   // operator delete, 0x0048ad60
        return 0;
    }

    int len = static_cast<int>(strlen(text));
    int cx = x;
    for (int i = 0; i < len; i++) {
        int advance = block->DrawGlyph(cx, y, hue, static_cast<uint8_t>(text[i]), dest, stride, clip);   // 0x00433bf0
        if (advance == 0)
            break;
        cx += advance;
    }
    if (copied != 0)
        operator delete(text);   // operator delete, 0x0048ad60
    return 1;
}
HOOK_METHOD(0x00432130, CFontManager, DrawTextPlain);

// 0x00432260  size 0x2ef
// Break the text into lines no wider than maxWidth (at the last space, else before the
// overflowing character), draw each line with DrawTextPlain below the previous ones, and return
// the total height. After a break the rest of the buffer is shifted down and `indent` spaces are
// put in front of it. lineHeight == 0: each line advances by its measured height.
// ? With indent > 0 the original writes up to `indent` bytes past the copy it allocated.
int __thiscall CFontManager::DrawTextWrapped(int x, int y, int font, int hue, char *text, int maxWidth, uint16_t *dest, int stride,
                                             const int *clip, int indent, int lineHeight)
{
    char line[0xc8];
    int accum = 0;
    int w, h, base;
    int brk;

    if (!(font <= 0x19 && font >= 0 && fontBlock[font] != 0))
        return 0;
    if (text == nullptr)
        return 0;

    char *buf = static_cast<char *>(::operator new(strlen(text) + 1));   // operator new, 0x0048ad70
    strcpy(buf, text);
    if (g_profanityOn != 0 && g_inputRedrawing == 0)
        ProfanityApply(buf);   // 0x00457460

    do {
        int lastSpace = -1;
        int width = 0;
        int len = static_cast<int>(strlen(buf));
        int i = 0;
        brk = -1;
        if (len > 0) {
            do {
                char c = buf[i];
                width += fmt_glyph_width(this, font, c);
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
            } while (i < len);
        }

        if (brk != -1) {
            int skipSpace = (buf[brk + 1] == ' ') ? 1 : 0;
            int count = brk + 1;
            memcpy(line, buf, static_cast<size_t>(count));
            line[count] = 0;
            MeasureText(font, line, &w, &h, &base);   // 0x004327b0
            DrawTextPlain(x, y + accum + base, font, hue, line, dest, stride, clip);   // 0x00432130
            accum += (lineHeight != 0) ? lineHeight : h;

            int rem = static_cast<int>(strlen(buf)) - skipSpace - count;
            int k = 0;
            if (rem > 0) {
                do {
                    buf[k] = buf[count + skipSpace + k];
                    k++;
                } while (k < rem);
            }
            buf[k] = 0;

            if (indent != 0) {
                buf[strlen(buf) + indent] = 0;
                int j = static_cast<int>(strlen(buf)) + indent - 1;
                if (j >= indent) {
                    do {
                        buf[j] = buf[j - indent];
                        j--;
                    } while (j >= indent);
                }
                if (indent > 0)
                    memset(buf, ' ', static_cast<size_t>(indent));
            }
        }
    } while (brk != -1);

    MeasureText(font, buf, &w, &h, &base);   // 0x004327b0
    DrawTextPlain(x, y + accum + base, font, hue, buf, dest, stride, clip);   // 0x00432130
    operator delete(buf);   // operator delete, 0x0048ad60
    return ((lineHeight != 0) ? lineHeight : h) + accum;
}
HOOK_METHOD(0x00432260, CFontManager, DrawTextWrapped);

// 0x00432550  size 0x254
// Draw one line. When x + width runs past clip[2] (the right edge) the copy of the text is
// shortened, one character at a time, until it plus "..." (three '.' glyphs) fits, and the dots
// are drawn after it. Glyph characters are signed here. The font index is not range checked at
// the start (fontBlock[font] is read directly).
int __thiscall CFontManager::DrawTextClipped(int x, int y, int font, int hue, char *text, uint16_t *dest, int stride, const int *clip)
{
    int width = 0;
    int ellipsis = 0;
    CFontBlock *block = reinterpret_cast<CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(fontBlock[font])));

    if (block == nullptr) {
        Fatal_Box("Missing Font!", "Error", 0);   // 0x00482c70
        return 0;
    }

    char *buf = static_cast<char *>(::operator new(strlen(text) + 1));   // operator new, 0x0048ad70
    strcpy(buf, text);
    text = buf;
    if (g_profanityOn != 0 && g_inputRedrawing == 0)
        ProfanityApply(text);   // 0x00457460

    int len = static_cast<int>(strlen(text));
    int i = 0;
    if (len > 0) {
        do {
            width += fmt_glyph_width(this, font, *(text + i));
            i++;
        } while (i < len);
    }

    int right = clip[2];
    if (width + x > right) {
        ellipsis = 1;
        int dots;
        if (fmt_font_ok(this, font) != 0)
            dots = *reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(reinterpret_cast<CFontBlock *>(static_cast<uintptr_t>(static_cast<uint32_t>(fontBlock[font])))->glyph[0x0e])));   // '.' glyph, block + 0x3c
        else
            dots = 0;
        dots = dots * 3;
        if (width + dots + x > right) {
            int limit = dots + x;
            do {
                int last = static_cast<signed char>(text[strlen(text) - 1]);
                width -= fmt_glyph_width(this, font, last);
                text[strlen(text) - 1] = 0;
            } while (width + limit > clip[2]);
        }
    }

    len = static_cast<int>(strlen(text));
    int cx = x;
    for (i = 0; i < len; i++)
        cx += block->DrawGlyph(cx, y, hue, static_cast<signed char>(text[i]), dest, stride, clip);   // 0x00433bf0
    if (ellipsis != 0) {
        for (int n = 3; n != 0; n--)
            cx += block->DrawGlyph(cx, y, hue, 0x2e, dest, stride, clip);   // 0x00433bf0
    }
    operator delete(text);   // operator delete, 0x0048ad60
    return 1;
}
HOOK_METHOD(0x00432550, CFontManager, DrawTextClipped);

// 0x004327b0  size 0x12f
// Width = sum of the advances of the characters >= 0x20; height = (largest ascent over the
// baseline) + (largest descent: glyph height minus baseline); base = descent - 1.
int __thiscall CFontManager::MeasureText(int font, char *text, int *outW, int *outH, int *outBase)
{
    int totalW = 0;
    int below = 0;     // largest (glyph height - baseline)
    int above = 0;     // largest baseline

    if (fmt_font_ok(this, font) == 0)
        return 0;

    CFontBlock *block = fmt_block(this, font);
    int len = static_cast<int>(strlen(text));
    for (int i = 0; i < len; i++) {
        uint8_t c = static_cast<uint8_t>(text[i]);
        if (c < 0x20)
            continue;
        int ch = c;
        int glyphW;
        if (fmt_block_ok(font, block) != 0 && ch >= 0x20 && ch - 0x20 < 0xe0)
            glyphW = reinterpret_cast<const uint16_t *>(block->glyph[ch - 0x20])[0];
        else
            glyphW = 0;
        totalW += glyphW;
        int glyphH;
        if (fmt_block_ok(font, block) != 0 && ch >= 0x20 && ch - 0x20 < 0xe0)
            glyphH = reinterpret_cast<const uint16_t *>(block->glyph[ch - 0x20])[1];
        else
            glyphH = 0;
        int baseline = block->baseline[ch - 0x20];
        if (glyphH - baseline > below)
            below = glyphH - baseline;
        if (baseline > above)
            above = baseline;
    }

    *outW = totalW;
    *outH = above + below;
    *outBase = below - 1;
    return 1;
}
HOOK_METHOD(0x004327b0, CFontManager, MeasureText);

// 0x00432ba0  size 0xcd
int __thiscall CFontManager::FindWrapBreak(int font, const char *text, int maxWidth)
{
    int i = 0;
    int lastSpace = -1;
    int width = 0;
    int len = static_cast<int>(strlen(text));

    for (; i < len; i++) {
        char c = text[i];
        width += fmt_glyph_width(this, font, c);
        if (c == ' ' && i > 1)
            lastSpace = i - 1;
        if (width > maxWidth) {
            if (lastSpace == -1)
                return i - 1;
            return lastSpace;
        }
    }
    return -1;
}
HOOK_METHOD(0x00432ba0, CFontManager, FindWrapBreak);
