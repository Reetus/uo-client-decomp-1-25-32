#include "spell_book_gump.h"

#include <cstdio>
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "app_session_ops_ports.h"
#include "font_manager.h"

// Text rects of the spellbook pages: {left, top, right, bottom} ints in the image.

namespace {

// Walk the book's spell list `idx` nodes from the head; spell id (0-based) or -1.
// Callers have already established that the book object answers slot 0x30 (spell container).
int SpellAtListIndex(CObj *book, int idx)
{
    char *node = RAWF(char *, book, 0xa8);
    int i = 0;
    if (idx > 0) {
        do {
            node = RAWF(char *, node, 0x40);
            if (node == nullptr)
                return -1;
            i++;
        } while (i < idx);
    }
    if (node == nullptr)
        return -1;
    return static_cast<int>(RAWF(uint16_t, node, 0x3c)) - 1;
}

} // namespace

HOOK_VTABLE(0x0049c850, CSpellBookGump);
HOOK_SDD(0x0040a720, CSpellBookGump);

// 0x0040a9d0  size 0x57
// Mouse up: the button is up; end a drag, or report the second click of a double click (slot 0x124).
void __thiscall CSpellBookGump::MouseUp(int mouseX, int mouseY)
{
    mouseDown = 0;
    if (unk_134 != 0) {
        unk_134 = 0;
        clickState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (clickState == 2) {
        clickState = 0;
        PORT_VSLOT2(this, 0x124, void, int, int)(mouseX, mouseY);
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0040a9d0, CSpellBookGump, MouseUp);

// 0x0040abb0  size 0x23c
// Single click: a circle tab (art 0x8b1.. at y 0xaf) goes to that circle's index page (an even
// result is moved back to the left page); the corner arts turn the page (slots 0x11c / 0x118); on an
// index page a click on a listed spell opens that spell's page.
void __thiscall CSpellBookGump::OnClickCorner(int mouseX, int mouseY)
{
    int prevW, prevH;   // art 0x8bb
    int nextW, nextH;   // art 0x8bc
    int tabW, tabH;     // art 0x8b1
    int tab = 0;
    BOOK_ArtSize(0x8bb, &prevW, &prevH);
    BOOK_ArtSize(0x8bc, &nextW, &nextH);
    BOOK_ArtSize(0x8b1, &tabW, &tabH);

    if (mouseY >= 0xaf && mouseY < tabH + 0xaf) {
        if (mouseX >= 0x3a && mouseX < tabW + 0x3a)
            tab = 1;
        if (mouseX >= 0x5d && mouseX < tabW + 0x5d)
            tab = 2;
        if (mouseX >= 0x82 && mouseX < tabW + 0x82)
            tab = 3;
        if (mouseX >= 0xa4 && mouseX < tabW + 0xa4)
            tab = 4;
        if (mouseX >= 0xe3 && mouseX < tabW + 0xe3)
            tab = 5;
        if (mouseX >= 0x104 && mouseX < tabW + 0x104)
            tab = 6;
        if (mouseX >= 0x129 && mouseX < tabW + 0x129)
            tab = 7;
        if (mouseX >= 0x14c && mouseX < tabW + 0x14c)
            tab = 8;
    }
    if (tab != 0) {
        page = static_cast<uint16_t>(tab);
        if (page % 2 == 0) {
            tab--;
            page = static_cast<uint16_t>(tab);
        }
        PORT_VSLOT0(this, 0x114, void)();
        return;
    }
    if (mouseX >= 0x32 && mouseX < prevW + 0x32 && mouseY >= 8 && mouseY < prevH + 8) {
        PORT_VSLOT0(this, 0x11c, void)();
        return;
    }
    if (mouseX >= 0x141 && mouseX < nextW + 0x141 && mouseY >= 8 && mouseY < nextH + 8) {
        PORT_VSLOT0(this, 0x118, void)();
        return;
    }
    if (page > indexPages)
        return;
    int spellIdx = GetIndexSpellAt(mouseX, mouseY);
    if (spellIdx < 0)
        return;
    if (indexPages % 2 == 0)
        page = static_cast<uint16_t>(spellIdx + indexPages + 1);
    else
        page = static_cast<uint16_t>(spellIdx + indexPages + 2);
    if (page % 2 == 0)
        page = static_cast<uint16_t>(page - 1);
    PORT_VSLOT0(this, 0x114, void)();
}
HOOK_VMETHOD(0x0040abb0, CSpellBookGump, OnClickCorner);

// 0x0040adf0  size 0x384
// Double click. Iconified (unk_60): restore. Otherwise: the corners jump to the first / last spread
// (with the page sound); a spell listed on an index page or shown on a spell page is selected: its
// "<spell+1> <book serial>" string goes to the server (0x0044df70, id 0x27), the gump iconifies and the
// last-spell global is set.
void __thiscall CSpellBookGump::OnCornerJump(int mouseX, int mouseY)
{
    int iconW, iconH;   // art 0x8c0
    int prevW, prevH;   // art 0x8bb
    int nextW, nextH;   // art 0x8bc
    char text[0x90];
    int spell;
    BOOK_ArtSize(0x8c0, &iconW, &iconH);
    BOOK_ArtSize(0x8bb, &prevW, &prevH);
    BOOK_ArtSize(0x8bc, &nextW, &nextH);

    if (unk_60 != 0) {
        Uniconify();
        return;
    }
    if (mouseX >= 0x32 && mouseX < prevW + 0x32 && mouseY >= 8 && mouseY < prevH + 8 && page > 1) {
        page = 1;
        PORT_VSLOT0(this, 0x114, void)();
        BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
        return;
    }
    if (mouseX >= 0x141 && mouseX < nextW + 0x141 && mouseY >= 8 && mouseY < nextH + 8) {
        page = pageCount;
        if ((pageCount & 1) == 0)
            page = static_cast<uint16_t>(pageCount - 1);
        PORT_VSLOT0(this, 0x114, void)();
        BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
        return;
    }
    int idxPages = indexPages;
    if (page <= indexPages) {
        // index page: the spell under the cursor
        int listIdx = GetIndexSpellAt(mouseX, mouseY);
        if (listIdx < 0)
            return;
        if (PORT_VSLOT0(owner, 0x30, int)() != 0)
            spell = SpellAtListIndex(owner, listIdx);
        else
            spell = -1;
    } else if (mouseX >= SB_RECT_L[0] && mouseX <= iconW + SB_RECT_L[0] && mouseY >= SB_RECT_L[1] + 0x1e &&
               mouseY <= iconH + SB_RECT_L[1] + 0x1e) {
        // left spell page
        int n;
        if ((idxPages & 1) != 0)
            n = static_cast<int>(page) - idxPages - 2;
        else
            n = static_cast<int>(page) - idxPages - 1;
        if (PORT_VSLOT0(owner, 0x30, int)() != 0)
            spell = SpellAtListIndex(owner, n);
        else
            spell = -1;
    } else if (mouseX >= SB_RECT_R[0] && mouseX <= iconW + SB_RECT_R[0] && mouseY >= SB_RECT_R[1] + 0x1e &&
               mouseY <= iconH + SB_RECT_R[1] + 0x1e) {
        // right spell page
        int n;
        if ((idxPages & 1) != 0)
            n = static_cast<int>(page) - idxPages - 2;
        else
            n = static_cast<int>(page) - idxPages - 1;
        n++;
        if (PORT_VSLOT0(owner, 0x30, int)() != 0)
            spell = SpellAtListIndex(owner, n);
        else
            spell = -1;
    } else {
        return;
    }
    // The original has this tail twice (index pages, spell pages): sprintf 0x0048b1d0, 0x0044df70, Iconify 0x0040d390.
    sprintf(text, "%d %d", spell + 1, RAWF(int32_t, owner, 0x78));
    SendTextPacket(0x27, text);   // send string request 0x27
    Iconify();
    g_lastSpell = spell;
    PORT_VSLOT0(this, 0x114, void)();
}
HOOK_VMETHOD(0x0040adf0, CSpellBookGump, OnCornerJump);

// 0x0040b180  size 0x5d
// Tick: a first click with no second click inside the double-click time becomes a single click
// (slot 0x120) if the button is up, else starts a drag (unk_134).
void __thiscall CSpellBookGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState == 1 && now - clickTick > dbl) {
        clickState = 0;
        if (mouseDown == 0) {
            PORT_VSLOT2(this, 0x120, void, int, int)(unk_6c, unk_70);
            return;
        }
        unk_134 = 1;
    }
}
HOOK_VMETHOD(0x0040b180, CSpellBookGump, Tick);

// 0x0040b1e0  size 0x59
// Hit test: true when (x, y) is inside the visible frame bitmap and its pixel is non-zero.
// The pixel is read at bitmap + (y * width + x) * 2, i.e. without skipping the 4-byte size header.
int __thiscall CSpellBookGump::HitTest(int localX, int localY)
{
    uint16_t *bmp;
    if (unk_60 != 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[3]));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    if (localX < 0 || localY < 0)
        return 0;
    if (localX >= bmp[0])
        return 0;
    if (localY >= bmp[1])
        return 0;
    return bmp[localY * bmp[0] + localX] != 0;
}
HOOK_VMETHOD(0x0040b1e0, CSpellBookGump, HitTest);

// 0x0040c510  size 0x14c
// On an index page (1..8): which entry of the book's spell list is drawn under (x, y). The rows are
// counted among the spells whose circle is this page (left column) or the next page (right column).
int __thiscall CSpellBookGump::GetIndexSpellAt(int mouseX, int mouseY)
{
    int w, lineH, d;
    int leftCol = 0;
    if (page < 1 || page > 8)
        return -1;
    if (mouseY < SB_RECT_L[1] || mouseY > SB_RECT_L[3])
        return -1;
    int px = mouseX;
    if (px >= SB_RECT_L[0] && px <= SB_RECT_L[2]) {
        leftCol = 1;   // left column (ebx in the original)
    } else if (px < SB_RECT_R[0] || px > SB_RECT_R[2]) {
        return -1;
    }
    lineH = 0;
    reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->MeasureText(9, (char *)(g_refText), &w, &lineH, &d);
    int row = (mouseY - SB_RECT_L[1] - 0x26) / lineH;
    int count = 0;
    int idx = 0;
    char *node = RAWF(char *, owner, 0xa8);
    if (node == nullptr)
        return -1;
    do {
        int id = RAWF(uint16_t, node, 0x3c);
        bool match;
        if (leftCol != 0) {
            match = ((id - 1) / 8 + 1) == static_cast<int>(page);
        } else {
            match = ((id - 1) / 8 + 1) == static_cast<int>(page) + 1;
        }
        if (match) {
            if (count == row)
                return idx;
            count++;
        }
        idx++;
        node = RAWF(char *, node, 0x40);
    } while (node != nullptr);
    return -1;
}
HOOK_METHOD(0x0040c510, CSpellBookGump, GetIndexSpellAt);
