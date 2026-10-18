#include "scroll_choice_gump.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "window_helpers_31_ports.h"
#include "globals.h"

#define g_appObject GLOBAL_PTR(void, g_app_storage)             // CApp; 0x00482be0 takes the cursor id

// 0x0041d620  size 0xbf
// Mouse move over the list: sets the cursor, updates the hover row and repaints when it changed.
void __thiscall CScrollChoiceGump::MouseOver(int px, int py)
{
    int w;
    int lineH;
    int off;
    int row;

    PORT_THISCALL1(&SetCursorForRegion, void, g_appObject, int)(0);
    if (px >= listLeft && px <= listRight && py >= listTop && py <= listBottom) {
        gfx::TextMeasure(5, g_refText, &w, &lineH, &off);
        row = (py - listTop) / lineH + firstRow;
        if (row >= count)
            row = -1;
        int old = g_scrollChoiceHoverRow;
        g_scrollChoiceHoverRow = row;
        if (row != old)
            Redraw();
        return;
    }
    if (g_scrollChoiceHoverRow != -1) {
        g_scrollChoiceHoverRow = -1;
        Redraw();
    }
}
HOOK_VMETHOD(0x0041d620, CScrollChoiceGump, MouseOver);

// 0x0041d700  size 0x79
// Double-click timer: when the double-click time has passed since tickStart the wait ends;
// while pendingClick is set the click counter reaches 2 and triggers OnLeftClick(+0x6c, +0x70).
void __thiscall CScrollChoiceGump::Tick()
{
    DWORD now = GetTickCount();
    UINT dct = GetDoubleClickTime();
    if (waitingDouble == 1 && (now - tickStart) > dct) {
        waitingDouble = 0;
        if (pendingClick != 0)
            unk_94 = 1;
    }
    if (pendingClick != 0) {
        clickCount = clickCount + 1;
        if (clickCount == 2) {
            clickCount = 0;
            this->CScrollChoiceGump::OnLeftClick(unk_6c, unk_70);   // OnLeftClick
        }
    }
}
HOOK_VMETHOD(0x0041d700, CScrollChoiceGump, Tick);

// 0x0041d780  size 0x2f
// Scrollbar callback: first visible entry = pos * count / range, clamped to count - 1; repaints.
void __thiscall CScrollChoiceGump::OnScroll(int pos, int range, int unused)
{
    (void)unused;
    firstRow = pos * count / range;
    if (firstRow >= count)
        firstRow = count - 1;
    Redraw();
}
HOOK_VMETHOD(0x0041d780, CScrollChoiceGump, OnScroll);

// 0x0041d7b0  size 0x71
// Appends an entry (id, hue, 0x50-byte name).
void __thiscall CScrollChoiceGump::AddEntry(int id, const char *entryName, uint16_t color)
{
    ids[count] = id;
    colors[count] = color;
    strcpy(names[count], entryName);
    count = count + 1;
}
HOOK_METHOD(0x0041d7b0, CScrollChoiceGump, AddEntry);

// 0x0041d830  size 0x29a
// Repaints frame0: background art twice, the wrapped title, then one text line per visible entry
// with the hover highlight (hue 0x21) and the three selection marks.
void __thiscall CScrollChoiceGump::Redraw()
{
    int rect[2] = {0, 0};   // [ebp-0x3c]: passed as the clip of the title text
    int w;
    int lineH;
    int lineOff;
    int yy;
    int row;
    int clip[4];
    uint16_t *pix = gfx::bitmapOf(this) + 2;

    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, pix, width, height);
    gfx::DrawArt(gfx::artPlain(), 0x944, 0x39, 0x10c, pix, width, height);

    if (strlen(reinterpret_cast<const char *>(pad_226c)) != 0)
        gfx::TextDrawWrapped(0x1c, height - 0x58, 6, 0, reinterpret_cast<const char *>(pad_226c), width - 0x28, pix, width,
                             rect, 0, 0xe);

    gfx::TextMeasure(5, g_refText, &w, &lineH, &lineOff);
    yy = listTop;
    row = firstRow;
    if (yy > listBottom)
        return;
    do {
        if (row >= count)
            return;
        clip[0] = 0;
        clip[1] = listTop;
        clip[2] = listRight;
        clip[3] = listBottom;
        if (ids[row] == mark[0])
            gfx::DrawArtClipped(gfx::artPlain(), 0x938, clip, 0x1a, yy + 10, pix, width, height);
        if (ids[row] == mark[1])
            gfx::DrawArtClipped(gfx::artPlain(), 0x939, clip, 0x28, yy + 10, pix, width, height);
        if (ids[row] == mark[2])
            gfx::DrawArtClipped(gfx::artPlain(), 0x93a, clip, 0x36, yy + 10, pix, width, height);
        int hue;
        if (row == g_scrollChoiceHoverRow) {
            hue = 0x21;
        } else {
            hue = colors[row];
            if (hue == 0)
                hue = 0x386;
        }
        gfx::TextDrawAlt(listLeft, lineOff + yy, 5, hue, names[row], pix, width, &listLeft);
        yy += lineH;
        row++;
    } while (yy <= listBottom);
}
HOOK_METHOD(0x0041d830, CScrollChoiceGump, Redraw);
