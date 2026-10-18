// Unifont (unifont.mul) loader, glyph lookup, and the single-line unicode draw / measure routines.
// The file is mapped through MappedFile; DAT_007581b0 is the view: 0x10000 dword offsets, then glyph
// records {int8 xoff, int8 yoff, int8 width, int8 height, bitmap rows of ceil(width/8) bytes}.
#include <cstdint>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "mapped_file.h"
#include "crt.h"


// 0x0047c920  size 0xad
// Load unifont.mul if the file can be opened.
void __cdecl LoadUnifont()
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("unifont.mul")), g_fopenReadMode);
    if (f != nullptr) {
        MappedFile *mf;

        fclose(f);
        mf = new MappedFile(g_app.ResolveDataPath(const_cast<char *>("unifont.mul")));
        uf_file = mf;
        g_unifontTable = static_cast<uint8_t *>(mf->OpenReadOnly());
    }
    g_unifontLoaded = 1;
}
HOOK_FUNC(0x0047c920, LoadUnifont);

// 0x0047c9d0  size 0xa3
// ECX = code point (low 16 bits), EDX = &xoff, stack (&yoff, &width, &height), RET 0xc.
// Returns the glyph bitmap, or 0 with all four outputs zeroed.
const uint8_t *__fastcall UnifontGetGlyph(uint16_t code, int *outX, int *outY, int *outW, int *outH)
{
    uint32_t off;
    const uint8_t *table;

    if (g_unifontLoaded == 0)
        LoadUnifont();
    table = g_unifontTable;
    if (table != nullptr) {
        off = reinterpret_cast<const uint32_t *>(table)[code];
        if (off != 0) {
            *outX = static_cast<int8_t>(g_unifontTable[off]);
            *outY = static_cast<int8_t>(g_unifontTable[off + 1]);
            *outW = static_cast<int8_t>(g_unifontTable[off + 2]);
            *outH = static_cast<int8_t>(g_unifontTable[off + 3]);
            return g_unifontTable + off + 4;
        }
    }
    *outH = 0;
    *outW = 0;
    *outY = 0;
    *outX = 0;
    return nullptr;
}
HOOK_FUNC(0x0047c9d0, UnifontGetGlyph);

// 0x0047ca80  size 0x5e
// ECX = code point; true when the font has a glyph (a space always counts).
static inline const uint8_t *uf_FindGlyph(uint16_t code)
{
    uint32_t off;
    const uint8_t *table;

    if (g_unifontLoaded == 0)
        LoadUnifont();
    table = g_unifontTable;
    if (table != nullptr) {
        off = reinterpret_cast<const uint32_t *>(table)[code];
        if (off != 0)
            return g_unifontTable + off + 4;
    }
    return nullptr;
}

int __fastcall UnifontHasGlyph(uint16_t code)
{
    if (code == 0x20)
        return 1;
    return uf_FindGlyph(code) != nullptr;
}
HOOK_FUNC(0x0047ca80, UnifontHasGlyph);

// 0x0047cae0  size 0x199
// ECX = dest pixels, EDX = pitch in pixels; stack (text, x, y, colour, clip{l,t,r,b}), RET 0x14.
// A space advances 8; a glyph is drawn at x + xoff, y + yoff and advances xoff + width + 1.
// A glyph row that is clipped out vertically does not consume its bitmap bytes.
// A character without a glyph ends the string.
void __fastcall DrawUnicodeString(uint16_t *dest, int pitch, const uint16_t *text, int x, int y,
                                         uint16_t color, const int *clip)
{
    uint8_t bits = 0;
    uint16_t ch;

    if (g_unifontLoaded == 0) {
        LoadUnifont();
        if (g_unifontLoaded == 0)
            return;
    }
    ch = *text++;
    if (ch == 0)
        return;
    do {
        if (ch == 0x20) {
            x += 8;
            ch = *text++;
            continue;
        }
        int gx, gy, gw, gh;
        const uint8_t *data;

        if (g_unifontLoaded == 0)
            LoadUnifont();
        {
            const uint8_t *table = g_unifontTable;
            uint32_t off = 0;
            if (table != nullptr)
                off = reinterpret_cast<const uint32_t *>(table)[ch];
            if (table != nullptr && off != 0) {
                gx = static_cast<int8_t>(table[off]);
                gy = static_cast<int8_t>(table[off + 1]);
                gw = static_cast<int8_t>(table[off + 2]);
                gh = static_cast<int8_t>(table[off + 3]);
                data = table + off + 4;
            } else {
                gx = 0;
                gy = 0;
                gw = 0;
                gh = 0;
                data = nullptr;
            }
        }
        if (data == nullptr)
            return;

        int px = x + gx;               // [ebp-0x20]
        int cury = gy + y;             // y was biased by +14 and the bias removed again
        int rowOff = cury * pitch;
        uint8_t rows = static_cast<uint8_t>(gh);
        int w8 = gw & 0xff;
        int cx = px;

        do {
            if (cury >= clip[1] && cury < clip[3]) {
                if (w8 > 0) {
                    int i = 0;
                    do {
                        int bit = i & 7;
                        if (bit == 0)
                            bits = *data++;
                        if ((bits & (1 << (7 - bit))) != 0 && cx >= clip[0] && cx < clip[2])
                            dest[cx + rowOff] = color;
                        cx++;
                        i++;
                    } while (i < w8);
                }
                cx -= w8;
            }
            cury++;
            rowOff += pitch;
            rows--;
        } while (rows != 0);

        x = px + w8 + 1;
        ch = *text++;
    } while (ch != 0);
}
HOOK_FUNC(0x0047cae0, DrawUnicodeString);

// 0x0047cc80  size 0xe1
// ECX = text, EDX = &width; stack (&height, &yOffset), RET 8. Sums the advances; height is always 0x12,
// the second output always 0. Characters without a glyph add nothing.
void __fastcall MeasureUnicodeString(const uint16_t *text, int *outWidth, int *outHeight, int *outYOff)
{
    int total = 0;

    for (; *text != 0; text++) {
        uint16_t ch = *text;
        int has;
        int add;

        if (ch == 0x20) {
            has = 1;
        } else {
            if (g_unifontLoaded == 0)
                LoadUnifont();
            has = (g_unifontTable != nullptr && reinterpret_cast<const uint32_t *>(g_unifontTable)[ch] != 0) ? 1 : 0;
        }
        if (has != 0) {
            ch = *text;
            if (ch == 0x20) {
                add = 8;
            } else {
                if (g_unifontLoaded == 0)
                    LoadUnifont();
                if (g_unifontTable != nullptr && reinterpret_cast<const uint32_t *>(g_unifontTable)[ch] != 0) {
                    uint32_t off = reinterpret_cast<const uint32_t *>(g_unifontTable)[ch];
                    add = static_cast<int8_t>(g_unifontTable[off + 2]) + static_cast<int8_t>(g_unifontTable[off]) + 1;
                } else {
                    add = 1;
                }
            }
            total += add;
        }
    }
    *outWidth = total;
    *outHeight = 0x12;
    *outYOff = 0;
}
HOOK_FUNC(0x0047cc80, MeasureUnicodeString);
