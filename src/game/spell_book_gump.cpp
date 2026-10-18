#include "spell_book_gump.h"

#include <cstdlib>
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "font_manager.h"
#include "book_gump.h"
#include "button_gump.h"
#include "gump.h"
#include "spell_icon_gump.h"
#include "vtables.h"

// Spell tables in the image: names (8-byte entries), descriptions and reagent digit strings (4-byte entries).
#define SPELL_NAME(i) GLOBAL_AS(char *, g_spellNames[(i) * 2])   // 8-byte entries, the name first
#define SPELL_DESC(i) (g_spellDescs[(i)])
#define SPELL_REAGENTS(i) (g_spellReagents[(i)])
// Text rects of the spellbook pages: {left, top, right, bottom} ints in the image.

namespace {

inline void Measure(int font, const char *str, int *w, int *a, int *b)
{
    reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->MeasureText(font, (char *)(str), w, a, b);
}
// 0x00432130 / 0x00432550: draw one text line. (x, y, font, hue, str, pixels, width, clip rect)
inline void DrawText130(int x, int y, int font, int hue, const char *str, void *pixels, int w, int *rect)
{
    reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->DrawTextPlain(x, y, font, hue, (char *)(str), (uint16_t *)(pixels), w, (const int *)(rect));
}
inline void DrawText550(int x, int y, int font, int hue, const char *str, void *pixels, int w, int *rect)
{
    reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->DrawTextClipped(x, y, font, hue, (char *)(str), (uint16_t *)(pixels), w, (const int *)(rect));
}
// 0x00432260: draw wrapped text; returns the height. (x, y, font, hue, str, wrapWidth, pixels, w, clip, 0, 0xe)
inline int DrawWrapped(int x, int y, int font, int hue, const char *str, int wrapWidth, void *pixels, int w, int *rect,
                       int a, int b)
{
    return reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->DrawTextWrapped(x, y, font, hue, (char *)(str), wrapWidth, (uint16_t *)(pixels), w, (const int *)(rect), a, b);
}

const char *CircleName(int idx)
{
    switch (static_cast<unsigned>(idx)) {
    case 0: return "First Circle";
    case 1: return "Second Circle";
    case 2: return "Third Circle";
    case 3: return "Fourth Circle";
    case 4: return "Fifth Circle";
    case 5: return "Sixth Circle";
    case 6: return "Seventh Circle";
    case 7: return "Eighth Circle";
    default: return "Bad Circle";
    }
}

const char *ReagentName(int idx)
{
    switch (static_cast<unsigned>(idx)) {
    case 0: return "Bloodmoss";
    case 1: return "Nightshade";
    case 2: return "Garlic";
    case 3: return "Ginseng";
    case 4: return "Mandrake root";
    case 5: return "Spiders' silk";
    case 6: return "Sulfurous ash";
    case 7: return "Black pearl";
    default: return "Bad reagent";
    }
}

// Copy a spell name up to (not including) the first '(' into the shared buffer at 0x004b70e0.
// The NUL goes to buf[count-1]: the last copied character is dropped (the asm stores to 0x4b70df + count).
void CopySpellName(const char *name)
{
    int cnt = 0;
    if (name != nullptr && static_cast<int>(strlen(name)) > 0) {
        const char *s = name;
        do {
            char ch = *s;
            if (ch == '(')
                break;
            cnt++;
            g_parenBuf[s - name] = ch;
            s++;
        } while (cnt < static_cast<int>(strlen(name)));
    }
    g_parenBuf[cnt - 1] = 0;
}

} // namespace

// 0x0040a450  size 0x2c3
// CSpellBookGump constructor. The SEH frame is omitted.
CSpellBookGump::CSpellBookGump(int posX, int posY, CObj *book)
{
    name = const_cast<char *>("spellbook gump");
    dirty = 0;
    unk_50 = 0x8ac;
    BOOK_ArtSize(0x8ac, &width, &height);
    height += 0x14;
    rowStride = width;
    BOOK_ArtSize(0x8ba, &unk_24, &unk_28);
    owner = book;
    if (PORT_VSLOT0(book, 0x30, int)() != 0)
        RAWF(CSpellBookGump *, owner, 0xac) = this;   // owner back pointer
    x = posX;
    y = posY;
    unk_60 = 0;
    selectedSpell = -1;
    page = 1;
    int spell = g_lastSpell;
    if (spell != -1) {
        for (char *it = RAWF(char *, owner, 0xa8); it != nullptr; it = RAWF(char *, it, 0x40)) {   // contained items
            if (static_cast<int>(RAWF(uint16_t, it, 0x3c)) - 1 == spell) {
                page = static_cast<uint16_t>(spell / 8 + 1);
                break;
            }
        }
    }
    unk_130 = 0;
    unk_140 = 0;
    unk_134 = 0;
    clickTick = 0;
    clickState = 0;
    mouseDown = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    frameIndex = 0;
    frameCount = 2;
    frameData = FrameData_Alloc(g_frameDataAllocator, 0x10, "framedata for CSpellbookGump");
    memset(frameData, 0, frameCount * 8);
    frameData[1] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(FrameData_Alloc(g_frameDataAllocator, width * height * 2 + 4, "course frame0")));
    memset(static_cast<uint8_t *>(i32_ptr(frameData[1])) + 4, 0, height * width * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[1])) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(i32_ptr(frameData[1])) + 2) = static_cast<uint16_t>(height);
    frameData[3] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(
        FrameData_Alloc(g_frameDataAllocator, unk_24 * unk_28 * 2 + 4, "spellbook frame1")));
    memset(static_cast<uint8_t *>(i32_ptr(frameData[3])) + 4, 0, unk_24 * unk_28 * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[3])) = static_cast<uint16_t>(unk_24);
    *reinterpret_cast<uint16_t *>(static_cast<uint8_t *>(i32_ptr(frameData[3])) + 2) = static_cast<uint16_t>(unk_28);
    BOOK_DrawArt(BOOK_ART_PLAIN, 0x8ba, 0, 0, static_cast<uint8_t *>(i32_ptr(frameData[3])) + 4, unk_24, unk_28);
    if (x == -1) {
        x = 0x140 - width / 2;   // centre on 640x480
        y = 0xf0 - height / 2;
    }
        CGump *btn;
    btn = (CGump *)new CButtonGump(3, 100, 0x15, 0x76, this, 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CBookGump::Iconify))), 0);
    CGump_AddChild(btn, this, 1);
    RedrawSpread();
    BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
}
HOOK_CTOR(0x0040a450, CSpellBookGump, (int, int, CObj *));

// 0x0040a750  size 0xb4
// ~CSpellBookGump. The SEH frame is omitted.
CSpellBookGump::~CSpellBookGump()
{
    int32_t *fd = frameData;
    if (fd != nullptr) {
        if (fd[1] != 0) {
            BOOK_FrameFree(g_frameDataAllocator, reinterpret_cast<void *>(static_cast<uintptr_t>(fd[1])),
                           "spellbook frame pBMP");
            frameData[1] = 0;
        }
        if (frameData[3] != 0) {
            BOOK_FrameFree(g_frameDataAllocator, reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[3])),
                           "spellbook frame2");
            frameData[3] = 0;
        }
        BOOK_FrameFree(g_frameDataAllocator, frameData, "spellbook framedata");
        frameData = nullptr;
    }
    BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
}
HOOK_DTOR(0x0040a750, CSpellBookGump);

// 0x0040a810  size 0x1bd
// Mouse down: record the press, bring the gump to front, count clicks, then pick the
// spell under the cursor into selectedSpell (-1 = none).
void __thiscall CSpellBookGump::MouseDown(int mouseX, int mouseY)
{
    mouseDown = 1;
    unk_6c = mouseX;   // press position (CGump +0x6c / +0x70)
    unk_70 = mouseY;
    this->CGump::BringToFront(0);   // bring to front
    if (clickState == 1) {
        clickState = 2;
    } else {
        clickState = 1;
        clickTick = GetTickCount();
    }
    int ox = mouseX, oy = mouseY;
    int w, h;
    if (page <= indexPages) {
        selectedSpell = -1;
        return;
    }
    BOOK_ArtSize(0x8c0, &w, &h);
    int count;
    if (ox >= SB_RECT_L[0] && ox <= w + SB_RECT_L[0] && oy >= SB_RECT_L[1] + 0x1e && oy <= h + SB_RECT_L[1] + 0x1e) {
        count = (static_cast<int>(page) - static_cast<int>(indexPages)) - 1;
    } else {
        int pg = page;
        int pages = pageCount;
        if (pg + 1 > pages || ox < SB_RECT_R[0] || ox > w + SB_RECT_R[0] || oy < SB_RECT_R[1] + 0x1e ||
            oy > h + SB_RECT_R[1] + 0x1e) {
            selectedSpell = -1;
            return;
        }
        count = pg - static_cast<int>(indexPages);
    }
    if (PORT_VSLOT0(owner, 0x30, int)() == 0) {
        selectedSpell = -1;
        return;
    }
    char *it = RAWF(char *, owner, 0xa8);
    int i = 0;
    if (count > 0) {
        do {
            it = RAWF(char *, it, 0x40);
            if (it == nullptr) {
                selectedSpell = -1;
                return;
            }
            i++;
        } while (i < count);
    }
    if (it == nullptr) {
        selectedSpell = -1;
        return;
    }
    selectedSpell = static_cast<int>(RAWF(uint16_t, it, 0x3c)) - 1;
}
HOOK_VMETHOD(0x0040a810, CSpellBookGump, MouseDown);

// 0x0040aa30  size 0x17e
// Mouse move while pressed: drag the window when no spell is selected; otherwise, after
// more than 4 px of movement, create (or re-grab) the spell icon gump and start dragging it.
void __thiscall CSpellBookGump::MouseMove(int mouseX, int mouseY)
{
    if (mouseDown == 0)
        return;
    if (selectedSpell == -1) {
        this->CGump::Move(mouseX - unk_6c, mouseY - unk_70, 1, 0, 1);
        return;
    }
    int dx = mouseX - unk_6c;
    if (abs(dx) <= 4) {
        int dy = mouseY - unk_70;
        if (abs(dy) <= 4)
            return;
    }
    this->CGump::LocalToScreen(&mouseX, &mouseY);
    CGump *icon = nullptr;
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (PORT_VSLOT0(g, 0x104, int)() != 0 && RAWF(int32_t, g, 0xac) == selectedSpell)
            icon = g;   // last match wins
    }
    if (icon == nullptr) {
        icon = new CSpellIconGump(mouseX - 0x14, mouseY - 0x14, selectedSpell);
        CGump_AddChild(icon, nullptr, 1);
    } else {
        icon->CGump::BringToFront(0);
    }
    RAWF(int32_t, icon, 0x9c) = 1;   // dragging
    RAWF(int32_t, icon, 0x6c) = icon->width / 2;
    RAWF(int32_t, icon, 0x70) = icon->height / 2;
    g_dragGump = icon;
    mouseDown = 0;
}
HOOK_VMETHOD(0x0040aa30, CSpellBookGump, MouseMove);

// The frame bitmap (header: u16 width, u16 height; the 16-bit pixels follow at +4).
static uint8_t *FrameBitmap(CSpellBookGump *g)
{
    return reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(g->frameData[1]));
}

static void *FramePixels(CSpellBookGump *g)
{
    return static_cast<void *>(FrameBitmap(g) + 4);
}

// Draw art `id` at (ax, ay) into the frame bitmap.
static void DrawFrameArt(CSpellBookGump *g, int id, int ax, int ay)
{
    uint8_t *b = FrameBitmap(g);
    BOOK_DrawArt(BOOK_ART_PLAIN, id, ax, ay, b + 4, *reinterpret_cast<uint16_t *>(b),
                 *reinterpret_cast<uint16_t *>(b + 2));
}

// Vtable slot 0x30 of the book's owner: true when the owner is a mobile (spell list read from it).
static bool IsMobileBook(CSpellBookGump *g)
{
    return PORT_VSLOT0(g->owner, 0x30, int)() != 0;
}

// 0x0040b2d0  size 0x1132
// RedrawFrame: clear the frame bitmap and draw the current spread. Pages 1..indexPages are
// index pages (spells of one circle per page); later pages show one spell each (icon, name,
// description, reagents).
// One spell page of RedrawFrame: circle title, name, description, icon, divider, reagent list, selection
// marker. sizeA_w, iconW, iconH and fontH7 are the art and font sizes RedrawFrame measured.
static void DrawSpellPage(CSpellBookGump *g, int *R, int sp, bool isRight, int sizeA_w, int iconW, int iconH,
                          int fontH7)
{
    int textW, lineH, unused2, y_;
    int circle = sp / 8;
    const char *str = CircleName(circle);
    Measure(9, str, &textW, &lineH, &unused2);
    {
        int tx = (R[2] - textW - R[0]) / 2 + R[0];
        if (!isRight) {
            if (tx < sizeA_w + 0x32)
                tx = sizeA_w + 0x32;
        } else {
            if (tx + textW > 0x141)
                tx = 0x141 - textW;
            if (tx < R[0])
                tx = R[0];
        }
        str = CircleName(circle);
        DrawText550(tx, fontH7 + R[1], 6, 0, str, FramePixels(g), g->width, R);
    }
    CopySpellName(SPELL_NAME(sp));
    lineH = DrawWrapped(iconW + R[0] + 2, R[1] + 0x18, 6, 0, g_parenBuf, R[2] - iconW - R[0] - 2, FramePixels(g), g->width, R,
                        0, 0xe);
    DrawWrapped(iconW + R[0] + 2, lineH + R[1] + 0x1c, 8, 0x288, SPELL_DESC(sp), R[2] - iconW - R[0] - 2, FramePixels(g), g->width,
                R, 0, 0xe);
    // spell icon (art object 0x4761e0, size = this->width/g->height)
    BOOK_DrawArt(BOOK_ART_ICON, sp + 0x8c0, R[0], R[1] + 0x1e, FramePixels(g), g->width, g->height);
    // divider strip made of art 0x835 tiles
    {
        uint8_t *b = FrameBitmap(g);
        int tileW, tileH;
        int px = R[0];
        int dy = iconH + R[1] + 0x22;
        BOOK_ArtSize(0x835, &tileW, &tileH);
        if (px <= R[2]) {
            do {
                if (tileW + px < R[2])
                    BOOK_DrawArt(BOOK_ART_PLAIN, 0x835, px, dy, b + 4, *reinterpret_cast<uint16_t *>(b),
                                 *reinterpret_cast<uint16_t *>(b + 2));
                else
                    BOOK_DrawArtClipped(BOOK_ART_PLAIN, 0x835, R, px, dy, b + 4, *reinterpret_cast<uint16_t *>(b),
                                        *reinterpret_cast<uint16_t *>(b + 2));
                px = px + tileW;
            } while (px <= R[2]);
        }
    }
    DrawText130(R[0], iconH + fontH7 + R[1] + 0x26, 6, 0, "Reagents:", FramePixels(g), g->width, R);
    y_ = iconH + R[1] + 0x36;
    {
        int i = 0;
        char *reg = SPELL_REAGENTS(sp);   // digits '1'..'8'
        if (static_cast<int>(strlen(reg)) > 0) {
            do {
                const char *rn = ReagentName(reg[i] - 0x31);
                DrawText130(R[0], fontH7 + y_, 9, 0x288, rn, FramePixels(g), g->width, R);
                y_ = y_ + 0xe;
                reg = SPELL_REAGENTS(sp);
                i++;
            } while (i < static_cast<int>(strlen(reg)));
        }
    }
    if (g_lastSpell == sp) {
        if (!isRight) {
            DrawFrameArt(g, 0x8ad, 0xb8, 2);
            DrawFrameArt(g, 0x8af, 0xb8, R[1] + 0x1e);
        } else {
            DrawFrameArt(g, 0x8ae, 0xcb, 3);
            DrawFrameArt(g, 0x8b0, 0xcb, R[1] + 0x1e);
        }
    }
}

void __thiscall CSpellBookGump::RedrawSpread()
{
    int sizeA_w, sizeA_h;     // art 0x8bb
    int iconW, iconH;         // art 0x8c0 (iconW += 4 below)
    int sizeC_w, sizeC_h;     // art (unk_50 + 2)
    int rect[4];              // clip {0, 0, width, height}
    int textW;
    int lineH;
    int fontH7;
    int unused1;
    int unused2;
    int y_;
    int spell;
    int leftIdx;
    int n;

    BOOK_ArtSize(0x8bb, &sizeA_w, &sizeA_h);
    BOOK_ArtSize(0x8c0, &iconW, &iconH);
    iconW = iconW + 4;
    BOOK_ArtSize(unk_50 + 2, &sizeC_w, &sizeC_h);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    Measure(7, g_refText, &textW, &unused1, &fontH7);

    spellCount = 0;
    indexPages = 8;
    {
        if (IsMobileBook(this)) {
            char *p = RAWF(char *, owner, 0xa8);
            if (p != nullptr) {
                do {
                    spellCount = static_cast<uint16_t>(spellCount + 1);
                    p = RAWF(char *, p, 0x40);
                } while (p != nullptr);
            }
        }
    }
    n = indexPages;
    if ((n % 2) != 0)
        n = n + 1;
    pageCount = static_cast<uint16_t>(n + spellCount);

    // clear the 16-bit surface: w*h*2 bytes starting at bitmap+4
    memset(FrameBitmap(this) + 4, 0, height * width * 2);

    // background
    {
        uint8_t *b = FrameBitmap(this);
        BOOK_DrawArt(BOOK_ART_PLAIN, unk_50, 0, 0, b + 4, *reinterpret_cast<uint16_t *>(b),
                     *reinterpret_cast<uint16_t *>(b + 2));
    }
    if (page > 1)   // previous page corner
        DrawFrameArt(this, 0x8bb, 0x32, 8);
    if (static_cast<int>(page) + 1 < static_cast<int>(pageCount))   // next page corner
        DrawFrameArt(this, 0x8bc, 0x141, 8);
    // circle tabs
    DrawFrameArt(this, 0x8b1, 0x3a, 0xaf);
    DrawFrameArt(this, 0x8b2, 0x5d, 0xaf);
    DrawFrameArt(this, 0x8b3, 0x82, 0xaf);
    DrawFrameArt(this, 0x8b4, 0xa4, 0xaf);
    DrawFrameArt(this, 0x8b5, 0xe3, 0xaf);
    DrawFrameArt(this, 0x8b6, 0x104, 0xaf);
    DrawFrameArt(this, 0x8b7, 0x129, 0xaf);
    DrawFrameArt(this, 0x8b8, 0x14c, 0xaf);

    if (page <= indexPages) {
        // ======================= index pages =======================
        // ---- left column ----
        DrawText130(SB_RECT_L[0] + 0x2c, fontH7 + 0xa, 6, 0, "INDEX", FramePixels(this), width, rect);
        const char *str = CircleName(static_cast<int>(page) - 1);
        DrawText130(SB_RECT_L[0], fontH7 + SB_RECT_L[1] + 0x14, 6, 0, str, FramePixels(this), width, rect);
        y_ = SB_RECT_L[1] + 0x24;
        {
            char *p = RAWF(char *, owner, 0xa8);
            while (p != nullptr) {
                int spellId = RAWF(uint16_t, p, 0x3c);
                int id = spellId - 1;
                if (id / 8 + 1 == static_cast<int>(page)) {
                    CopySpellName(SPELL_NAME(id));
                    Measure(9, g_refText, &textW, &lineH, &unused2);
                    DrawText550(SB_RECT_L[0], fontH7 + y_, 9, 0x288, g_parenBuf, FramePixels(this), width, SB_RECT_L);
                    if (g_lastSpell == id) {
                        DrawFrameArt(this, 0x8ad, 0xb8, 2);
                        DrawFrameArt(this, 0x8af, 0xb8, y_ + 6);
                    }
                    y_ = y_ + lineH;
                }
                p = RAWF(char *, p, 0x40);
            }
        }

        // ---- right column ----
        DrawText130(SB_RECT_R[0] + 0x2c, fontH7 + 0xa, 6, 0, "INDEX", FramePixels(this), width, rect);
        str = CircleName(static_cast<int>(page));
        DrawText130(SB_RECT_R[0], fontH7 + SB_RECT_R[1] + 0x14, 6, 0, str, FramePixels(this), width, rect);
        y_ = SB_RECT_R[1] + 0x24;
        {
            char *p = RAWF(char *, owner, 0xa8);
            while (p != nullptr) {
                int spellId = RAWF(uint16_t, p, 0x3c);
                int id = spellId - 1;
                if (id / 8 + 1 == static_cast<int>(page) + 1) {
                    CopySpellName(SPELL_NAME(id));
                    Measure(9, g_refText, &textW, &lineH, &unused2);
                    DrawText550(SB_RECT_R[0], fontH7 + y_, 9, 0x288, g_parenBuf, FramePixels(this), width, SB_RECT_R);
                    if (g_lastSpell == id) {
                        DrawFrameArt(this, 0x8ae, 0xcb, 3);
                        DrawFrameArt(this, 0x8b0, 0xcb, y_ + 6);
                    }
                    y_ = y_ + lineH;
                }
                p = RAWF(char *, p, 0x40);
            }
        }
        return;
    }

    // ======================= spell pages =======================
    leftIdx = static_cast<int>(page) - n - 1;

    // ---- left page ----
    if (spellCount != 0) {
        spell = -1;
        if (IsMobileBook(this)) {
            int k = 0;
            char *p = RAWF(char *, owner, 0xa8);
            if (leftIdx > 0) {
                do {
                    p = RAWF(char *, p, 0x40);
                    if (p == nullptr)
                        break;
                    k++;
                } while (k < leftIdx);
            }
            if (p != nullptr)
                spell = static_cast<int>(RAWF(uint16_t, p, 0x3c)) - 1;
        }
        if (spell >= 0)
            DrawSpellPage(this, SB_RECT_L, spell, false, sizeA_w, iconW, iconH, fontH7);
    }

    // ---- right page ----
    if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount)) {
        int idx = leftIdx + 1;
        if (IsMobileBook(this)) {
            int k = 0;
            char *p = RAWF(char *, owner, 0xa8);
            if (idx > 0) {
                do {
                    p = RAWF(char *, p, 0x40);
                    if (p == nullptr)
                        break;
                    k++;
                } while (k < idx);
            }
            if (p != nullptr)
                spell = static_cast<int>(RAWF(uint16_t, p, 0x3c)) - 1;
            else
                spell = -1;
        } else {
            spell = -1;
        }
        if (spell >= 0)
            DrawSpellPage(this, SB_RECT_R, spell, true, sizeA_w, iconW, iconH, fontH7);
    }
}
HOOK_VMETHOD(0x0040b2d0, CSpellBookGump, RedrawSpread);
